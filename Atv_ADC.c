#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3c

#define GPIO_BUTTON_A 5
#define GPIO_BUTTON_B 6 // para o modo bootloader
#define GPIO_LED_GREEN 11
#define GPIO_LED_RED 13
#define GPIO_LED_BLUE 12
#define BUTTON_JOY 22

const uint16_t PERIODO = 4095; // periodo do pwm de acrodo com a faixa de atuacao do joystick
const float DIVISOR = 15; // divisor fracionario do pwm
    
volatile bool anima_ativa = false;
volatile bool quadrado_visivel = true;
#define VRX_PIN 26
#define VRY_PIN 27

ssd1306_t ssd; //inicializa a estrutura do display, esta aqui para evitar um erro na compilacao

void setup_pwm(uint pin){ // inicializa e configura o pwm
  uint slice;
  gpio_set_function(pin, GPIO_FUNC_PWM);// Configura o pino do LED para função PWM
  slice = pwm_gpio_to_slice_num(pin); // obtendo o numeor do slice de cada pino
  pwm_set_clkdiv(slice, DIVISOR); // setando o divisor de clock
  pwm_set_wrap(slice, PERIODO); // configurando o periodo
  pwm_set_gpio_level(pin, 0); // inicialmente apagado 
  pwm_set_enabled(slice, true); // habilitando o pwm
}

void setup_led_and_button() { // para iniciar e configurar os botes e leds
    // Inicializa e configura o boatao A
    gpio_init(GPIO_BUTTON_A);
    gpio_set_dir(GPIO_BUTTON_A,GPIO_IN);
    gpio_pull_up(GPIO_BUTTON_A);
    // Inicializa e configura o boatao B
    gpio_init(GPIO_BUTTON_B);
    gpio_set_dir(GPIO_BUTTON_B,GPIO_IN);
    gpio_pull_up(GPIO_BUTTON_B);

    //inicializa o botao joy
    gpio_init(BUTTON_JOY);
    gpio_set_dir(BUTTON_JOY,GPIO_IN);
    gpio_pull_up(BUTTON_JOY);
    // inicializa e configura os leds 
    gpio_init(GPIO_LED_GREEN);
    gpio_set_dir(GPIO_LED_GREEN,GPIO_OUT);

}


// callback da interrupcao do botao do joystick
static volatile uint32_t last_time = 0;
bool estado_led_green = false;
 bool habilidato = true;
void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    
    
    if (gpio == BUTTON_JOY) {
        if (current_time - last_time>200000){ //200 ms de deboucing
            last_time = current_time; // atualiza o tempo do ultimo evento
            estado_led_green= !estado_led_green;
            gpio_put(GPIO_LED_GREEN, estado_led_green); // atualiza o estado do led
            quadrado_visivel = false;
            anima_ativa = false;
            //mudanca na borda do display
            // limpa o display e inicializa a moldura grossa
           for (int i= 0; i<= 5; i++){
            ssd1306_fill(&ssd,false);
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);
            ssd1306_rect(&ssd, 1, 1, 126, 62, true, false);
            ssd1306_rect(&ssd, 2, 2, 124, 60, true, false);
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
            ssd1306_rect(&ssd, 4, 4, 120, 56, true, false);
            ssd1306_send_data(&ssd);
           }
          anima_ativa = true;
          quadrado_visivel = true;  // Quadrado continua visível após animação
        }
    } if (gpio == GPIO_BUTTON_B){
        printf("Ativando modo de gravacao!\n");
        reset_usb_boot(0,0);
    } else if (gpio == GPIO_BUTTON_A){
       habilidato = !habilidato;
        pwm_set_enabled(6, habilidato);
    }
}



int main()
{
    stdio_init_all(); //inicialia a comunicacao serial
    setup_led_and_button(); //inicializa os leds e o botao A
    setup_pwm(GPIO_LED_BLUE); // configura o pwm para o led azul
    setup_pwm(GPIO_LED_RED); // configura o pwm para o led vermelho

    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    //Inicializacao e configuracao do I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // configura o pino gpio para a funcao serial data line do i2c
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // configura o pino gpio para a funcao clock data line do i2c
    gpio_pull_up(I2C_SDA); // pull up no pino de data line 
    gpio_pull_up(I2C_SCL); // pull up no pino de clock line
    
    //inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // inicializa o display
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);



    //interrupcao do botao do joystick
    gpio_set_irq_enabled_with_callback(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(GPIO_BUTTON_B, GPIO_IRQ_EDGE_FALL, true); // para acionar o bootloader no botao B
    gpio_set_irq_enabled(GPIO_BUTTON_A, GPIO_IRQ_EDGE_FALL, true);

    uint16_t faixa_led_apagado = 16; // uma faixa segura para corrigir o erro de centro do joystick
    int centro_display_x = 128  / 2;  // Centro X ajustado
    int centro_display_y = 64 / 2;   // Centro Y ajustado
    

    while (true) {
      if(anima_ativa)
      adc_select_input(1);
       uint16_t valor_x = adc_read(); // le o valor do eixo x do joystick
       uint16_t dutty_cycle_x = (abs(valor_x - 2048)> faixa_led_apagado) ? abs(valor_x - 2048) : 0; // calcula a distancia sem sinal negativo ate o centro 
       pwm_set_gpio_level(GPIO_LED_RED, dutty_cycle_x);

       adc_select_input(0);
       uint16_t valor_y = adc_read(); // le o valor do eixo y do joystick
       uint16_t dutty_cycle_y = (abs(valor_y - 2048)> faixa_led_apagado) ? abs(valor_y - 2048) : 0; // calcula a distancia sem sinal negativo ate o centro 
       pwm_set_gpio_level(GPIO_LED_BLUE, dutty_cycle_y);


     // Ampliando a movimentação com direção corrigida
     int deslocamento_x = (2048 - valor_x) / 70;  // Invertendo o eixo X
     int deslocamento_y = (2048 - valor_y) / 80;  // Invertendo o eixo Y

    // Limites ajustados para garantir que o quadrado não ultrapasse a borda
    int novo_x = centro_display_x + deslocamento_x;
    int novo_y = centro_display_y + deslocamento_y;

    if (novo_x < 0) novo_x = 0;
    if (novo_x > 119) novo_x = 119;  // 128 - 9 para borda
    if (novo_y < 0) novo_y = 0;
    if (novo_y > 55) novo_y = 55;    // 64 - 9 para borda
    
    if (quadrado_visivel) {
    ssd1306_fill(&ssd, false);
    draw_centered_square(&ssd, novo_x, novo_y, 8, true);  // Usa a função corrigida
    ssd1306_rect(&ssd, 0, 0, 128, 60, true, false);        // Borda do display
    ssd1306_send_data(&ssd);
    }

   }
 }
