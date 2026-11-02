#include "picoir/pico_backend.h"
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/platform.h"
#include "pico/time.h"
#include "ir_rx_sampler.pio.h"
#include "ir_tx_carrier.pio.h"
static bool valid_sm_pair(uint rx_sm,uint tx_sm){return rx_sm<4u&&tx_sm<4u&&rx_sm!=tx_sm;}
bool pir_pico_backend_init(pir_pico_backend_t *b,PIO pio,uint rx_sm,uint tx_sm,uint rx_pin,uint tx_pin,uint32_t sample_rate_hz,uint32_t carrier_hz){
    if(!b||!pio||!valid_sm_pair(rx_sm,tx_sm)||rx_pin>=30u||tx_pin>=30u||rx_pin==tx_pin||sample_rate_hz==0||sample_rate_hz>1000000u||carrier_hz==0||(uint64_t)sample_rate_hz*2u>clock_get_hz(clk_sys)||(uint64_t)carrier_hz*5u>clock_get_hz(clk_sys))return false;
    memset(b,0,sizeof(*b)); b->rx_dma_channel=-1; b->tx_dma_channel=-1; b->pio=pio;b->rx_sm=rx_sm;b->tx_sm=tx_sm;b->rx_pin=rx_pin;b->tx_pin=tx_pin;b->sample_rate_hz=sample_rate_hz;b->carrier_hz=carrier_hz;
    if(!pio_can_add_program(pio,&picoir_ir_rx_sampler_program)||!pio_can_add_program(pio,&picoir_ir_tx_carrier_program))return false;
    b->rx_offset=pio_add_program(pio,&picoir_ir_rx_sampler_program);b->tx_offset=pio_add_program(pio,&picoir_ir_tx_carrier_program);
    pio_gpio_init(pio,rx_pin);pio_sm_set_consecutive_pindirs(pio,rx_sm,rx_pin,1,false);
    pio_sm_config rx_cfg=picoir_ir_rx_sampler_program_get_default_config(b->rx_offset);sm_config_set_in_pins(&rx_cfg,rx_pin);sm_config_set_in_shift(&rx_cfg,false,true,32);sm_config_set_clkdiv(&rx_cfg,(float)clock_get_hz(clk_sys)/((float)sample_rate_hz*2.0f));pio_sm_init(pio,rx_sm,b->rx_offset,&rx_cfg);
    pio_gpio_init(pio,tx_pin);pio_sm_set_consecutive_pindirs(pio,tx_sm,tx_pin,1,true);
    pio_sm_config tx_cfg=picoir_ir_tx_carrier_program_get_default_config(b->tx_offset);sm_config_set_set_pins(&tx_cfg,tx_pin,1);sm_config_set_out_shift(&tx_cfg,false,false,32);sm_config_set_clkdiv(&tx_cfg,(float)clock_get_hz(clk_sys)/((float)carrier_hz*5.0f));pio_sm_init(pio,tx_sm,b->tx_offset,&tx_cfg);pio_sm_set_enabled(pio,tx_sm,false);pio_sm_set_pins_with_mask(pio,tx_sm,0u,1u<<tx_pin);b->initialized=true;return true;
}
bool pir_pico_rx_start(pir_pico_backend_t *b,uint32_t *sample_words,size_t word_count){
    if(!b||!b->initialized||!sample_words||word_count==0||word_count>UINT32_MAX||b->rx_running||b->tx_running)return false;
    int ch=dma_claim_unused_channel(false);if(ch<0)return false;b->rx_dma_channel=ch;dma_channel_config cfg=dma_channel_get_default_config((uint)ch);channel_config_set_transfer_data_size(&cfg,DMA_SIZE_32);channel_config_set_read_increment(&cfg,false);channel_config_set_write_increment(&cfg,true);channel_config_set_dreq(&cfg,pio_get_dreq(b->pio,b->rx_sm,false));pio_sm_clear_fifos(b->pio,b->rx_sm);pio_sm_restart(b->pio,b->rx_sm);dma_channel_configure((uint)ch,&cfg,sample_words,&b->pio->rxf[b->rx_sm],(uint32_t)word_count,false);pio_sm_set_enabled(b->pio,b->rx_sm,true);dma_start_channel_mask(1u<<(uint)ch);b->rx_running=true;return true;
}
bool pir_pico_rx_done(pir_pico_backend_t *b){if(!b||!b->rx_running||b->rx_dma_channel<0)return false;if(dma_channel_is_busy((uint)b->rx_dma_channel))return false;pio_sm_set_enabled(b->pio,b->rx_sm,false);dma_channel_unclaim((uint)b->rx_dma_channel);b->rx_dma_channel=-1;b->rx_running=false;return true;}
void pir_pico_rx_abort(pir_pico_backend_t *b){if(!b||!b->initialized)return;pio_sm_set_enabled(b->pio,b->rx_sm,false);if(b->rx_dma_channel>=0){dma_channel_abort((uint)b->rx_dma_channel);dma_channel_unclaim((uint)b->rx_dma_channel);b->rx_dma_channel=-1;}b->rx_running=false;pio_sm_clear_fifos(b->pio,b->rx_sm);}
bool pir_pico_tx_start(pir_pico_backend_t *b,const pir_pico_tx_run_t *runs,size_t run_count){
    if(!b||!b->initialized||!runs||run_count==0||run_count>PIR_PICO_TX_MAX_RUNS||b->tx_running||b->rx_running||b->tx_dma_channel>=0)return false;
    for(size_t i=0;i<run_count;i++){if(runs[i].duration_us==0)return false;uint64_t periods=((uint64_t)runs[i].duration_us*b->carrier_hz+500000u)/1000000u;if(periods==0)periods=1;if(periods>0x7fffffffu)return false;b->tx_words[i]=(runs[i].carrier_on?0x80000000u:0u)|((uint32_t)periods-1u);}
    int ch=dma_claim_unused_channel(false);if(ch<0)return false;b->tx_dma_channel=ch;dma_channel_config cfg=dma_channel_get_default_config((uint)ch);channel_config_set_transfer_data_size(&cfg,DMA_SIZE_32);channel_config_set_read_increment(&cfg,true);channel_config_set_write_increment(&cfg,false);channel_config_set_dreq(&cfg,pio_get_dreq(b->pio,b->tx_sm,true));pio_sm_clear_fifos(b->pio,b->tx_sm);pio_sm_restart(b->pio,b->tx_sm);dma_channel_configure((uint)ch,&cfg,&b->pio->txf[b->tx_sm],b->tx_words,(uint32_t)run_count,false);pio_sm_set_enabled(b->pio,b->tx_sm,true);dma_start_channel_mask(1u<<(uint)ch);b->tx_started_us=time_us_64();uint64_t total_us=0;for(size_t i=0;i<run_count;i++)total_us+=runs[i].duration_us;uint64_t margin_us=((2000000ull+b->carrier_hz-1u)/b->carrier_hz)*run_count+50u;b->tx_expected_done_us=b->tx_started_us+total_us+margin_us;b->tx_running=true;return true;
}
bool pir_pico_tx_done(pir_pico_backend_t *b){if(!b||!b->tx_running||b->tx_dma_channel<0)return false;if(dma_channel_is_busy((uint)b->tx_dma_channel))return false;if(time_us_64()<b->tx_expected_done_us||!pio_sm_is_tx_fifo_empty(b->pio,b->tx_sm))return false;pio_sm_set_enabled(b->pio,b->tx_sm,false);pio_sm_set_pins_with_mask(b->pio,b->tx_sm,0u,1u<<b->tx_pin);dma_channel_unclaim((uint)b->tx_dma_channel);b->tx_dma_channel=-1;b->tx_running=false;return true;}
void pir_pico_tx_abort(pir_pico_backend_t *b){if(!b||!b->initialized)return;pio_sm_set_enabled(b->pio,b->tx_sm,false);pio_sm_set_pins_with_mask(b->pio,b->tx_sm,0u,1u<<b->tx_pin);if(b->tx_dma_channel>=0){dma_channel_abort((uint)b->tx_dma_channel);dma_channel_unclaim((uint)b->tx_dma_channel);b->tx_dma_channel=-1;}b->tx_running=false;pio_sm_clear_fifos(b->pio,b->tx_sm);}
