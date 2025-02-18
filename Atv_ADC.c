#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

// ** Definições I2C **
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3c

const uint16_t PERIODO = 4095; // Periodo do pwm de acrodo com a faixa de atuacao do joystick
const float DIVISOR = 15; // Divisor fracionario do pwm

// ** Definições GPIO **
#define GPIO_BUTTON_A 5
#define GPIO_BUTTON_B 6 // para o modo bootloader
#define GPIO_LED_GREEN 11
#define GPIO_LED_RED 13
#define GPIO_LED_BLUE 12
#define BUTTON_JOY 22


// Variaveis de controle das animacoes da borda e quadrado central
volatile bool animacao = true;
volatile bool quadrado_visivel = true;

//Definicoes de pinos para leitura do joystick
#define VRX_PIN 26
#define VRY_PIN 27

ssd1306_t ssd; //inicializa a estrutura do display, esta aqui para evitar um erro na compilacao

// ** Inicializa e configura o pwm **
void setup_pwm(uint pin){ 
  uint slice;
  gpio_set_function(pin, GPIO_FUNC_PWM);// Configura o pino do LED para função PWM
  slice = pwm_gpio_to_slice_num(pin); // obtendo o numeor do slice de cada pino
  pwm_set_clkdiv(slice, DIVISOR); // setando o divisor de clock
  pwm_set_wrap(slice, PERIODO); // configurando o periodo
  pwm_set_gpio_level(pin, 0); // inicialmente apagado 
  pwm_set_enabled(slice, true); // habilitando o pwm
}

// ** Inicializa e configurar os botes e leds
void setup_led_and_button() { 
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


// ** Callback da interrupcao dos botoes **
static volatile uint32_t last_time_joy = 0; // Variavel de tempo passado para debouncing
static volatile uint32_t last_time_B = 0;
static volatile uint32_t last_time_A = 0;

bool estado_led_green = false; // Variavel de controle do led verde no botao do Joystick

bool habilidato = true; // variavel de controle do PWM no botao A

void gpio_irq_handler(uint gpio, uint32_t events){
   
    uint32_t current_time_joy = to_us_since_boot(get_absolute_time()); // Pega o tempo absoluto atual e guarda na variavel para comparacao
    
      if (gpio == BUTTON_JOY) { // Muda o estado do led verde, muda a animacao da borda rapidamente deixando maior.
       
        if (current_time_joy - last_time_joy>200000){ //200 ms de deboucing
            
            last_time_joy = current_time_joy; // Atualiza o tempo do ultimo evento
           
            estado_led_green= !estado_led_green; // Muda o estado do led verde
            gpio_put(GPIO_LED_GREEN, estado_led_green); // atualiza o estado do led

            quadrado_visivel = false; // Desativa a animacao da borda fina inicial, para a animacao da morda maior
            animacao= false; // Desativa a captacao de dados por um breve instante

            // limpa o display e inicializa a mudanca na borda
            for (int i= 0; i<= 5; i++){
            ssd1306_fill(&ssd,false);
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);
            ssd1306_rect(&ssd, 1, 1, 126, 62, true, false);
            ssd1306_rect(&ssd, 2, 2, 124, 60, true, false);
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
            ssd1306_rect(&ssd, 4, 4, 120, 56, true, false);
            ssd1306_send_data(&ssd);
            }
           
           animacao= true; // Ativa novamente a captacao de dados
           quadrado_visivel = true;  // Quadrado continua visível após animação
        }
    } if (gpio == GPIO_BUTTON_B){ // Funcionalidade extra para acionamento do modo bootloader
       
        uint32_t current_time_B = to_us_since_boot(get_absolute_time());
          
          if (current_time_B - last_time_B>200000){ //200 ms de deboucing 
            // nao atuliza o tempo passado pois ira entrar em bootloader
            printf("Ativando modo de gravacao!\n");
            reset_usb_boot(0,0);
          }
    } else if (gpio == GPIO_BUTTON_A){ // Ativa e desativa o PWM do led
        
        uint32_t current_time_A = to_us_since_boot(get_absolute_time());
        
        if (current_time_A - last_time_A>200000){ //200 ms de deboucing 
          
          last_time_A = current_time_A; // Atualiza o tempo do ultimo evento
          habilidato = !habilidato; // Muda o estado da variavel de controle do PWM
          pwm_set_enabled(6, habilidato);
        }
    }
}



int main()
{
    stdio_init_all(); //inicialia a comunicacao serial
    setup_led_and_button(); //inicializa os leds e o botao A
    setup_pwm(GPIO_LED_BLUE); // configura o pwm para o led azul
    setup_pwm(GPIO_LED_RED); // configura o pwm para o led vermelho

    adc_init(); // Inicializa o periferico ADC
    adc_gpio_init(VRX_PIN); // Inicializa o pino do eixo x
    adc_gpio_init(VRY_PIN); // Inicializa o pino do eixo y

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



    // ** Interrupcoes dos botoes **
    gpio_set_irq_enabled_with_callback(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Interrupcao do botao do joystick
    gpio_set_irq_enabled(GPIO_BUTTON_B, GPIO_IRQ_EDGE_FALL, true); // Para acionar o bootloader no botao B
    gpio_set_irq_enabled(GPIO_BUTTON_A, GPIO_IRQ_EDGE_FALL, true); // Para mudanca de estado do PWM

    uint16_t faixa_led_apagado = 16; // uma faixa segura para corrigir o erro de centro do joystick
    int centro_display_x = 128  / 2;  // Centro X ajustado
    int centro_display_y = 64 / 2;   // Centro Y ajustado
    

      while (true) {
          if (animacao){
          adc_select_input(1); //Adiciona o canal de entrada do ADC
          uint16_t valor_x = adc_read(); // le o valor do eixo x do joystick
          uint16_t dutty_cycle_x = (abs(valor_x - 2048)> faixa_led_apagado) ? abs(valor_x - 2048) : 0; // calcula a distancia sem sinal negativo ate o centro 
          pwm_set_gpio_level(GPIO_LED_RED, dutty_cycle_x); // Configura o Dutty cycle de acordo os dados lidos

          adc_select_input(0); //Adiciona o canal de entrada do ADC
          uint16_t valor_y = adc_read(); // le o valor do eixo y do joystick
          uint16_t dutty_cycle_y = (abs(valor_y - 2048)> faixa_led_apagado) ? abs(valor_y - 2048) : 0; // calcula a distancia sem sinal negativo ate o centro 
          pwm_set_gpio_level(GPIO_LED_BLUE, dutty_cycle_y); // Configura o Dutty cycle de acordo os dados lidos


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
            ssd1306_fill(&ssd, false); // Limpa o display
            draw_centered_square(&ssd, novo_x, novo_y, 8, true);  // Usa a função nova de criacao de borda (sem bugs)
            ssd1306_rect(&ssd, 0, 0, 128, 60, true, false);        // Borda do display fina defaut
            ssd1306_send_data(&ssd); // Atualiza o display
          }
          }
        
      }  
}
