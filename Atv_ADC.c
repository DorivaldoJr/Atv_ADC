#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/bootrom.h"

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
ssd1306_t ssd; //inicializa a estrutura do display, esta aqui para evitar um erro na compilacao

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
    gpio_init(GPIO_LED_RED);
    gpio_set_dir(GPIO_LED_RED,GPIO_OUT);
    gpio_init(GPIO_LED_BLUE);
    gpio_set_dir(GPIO_LED_BLUE,GPIO_OUT);
}
int64_t display_basico(alarm_id_t id, void *user_data);

// callback da interrupcao do botao do joystick
static volatile uint32_t last_time = 0;
bool estado_led_green = false;

void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    
    if (gpio == BUTTON_JOY) {
        if (current_time - last_time>200000){ //200 ms de deboucing
            last_time = current_time; // atualiza o tempo do ultimo evento
            estado_led_green= !estado_led_green;
            gpio_put(GPIO_LED_GREEN, estado_led_green); // atualiza o estado do led
    
            //mudanca na borda do display
            // limpa o display e inicializa a moldura grossa
            ssd1306_fill(&ssd,false);
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);
            ssd1306_rect(&ssd, 1, 1, 126, 62, true, false);
            ssd1306_rect(&ssd, 2, 2, 124, 60, true, false);
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
            ssd1306_rect(&ssd, 4, 4, 120, 56, true, false);
            ssd1306_send_data(&ssd);
       
        add_alarm_in_ms(800, display_basico, NULL, false);
        }
    } else if (gpio == GPIO_BUTTON_B){
        printf("Ativando modo de gravacao!\n");
        reset_usb_boot(0,0);
    }
}

// alarme para voltar o display normal depois do acionamento do botao
int64_t display_basico(alarm_id_t id, void *user_data){
    
    ssd1306_fill(&ssd,false);
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // desenha o quadrado basico
    ssd1306_send_data(&ssd);
    return 0;
}

int main()
{
    stdio_init_all(); //inicialia a comunicacao serial
    setup_led_and_button(); //inicializa os leds e o botao A
    
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

    // limpa o display e inicializa apagado
    ssd1306_fill(&ssd,false);
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // desenha o quadrado basico
    ssd1306_send_data(&ssd);

    //interrupcao do botao do joystick
    gpio_set_irq_enabled_with_callback(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(GPIO_BUTTON_B, GPIO_IRQ_EDGE_FALL, true); // para acionar o bootloader no botao B
    
    while (true) {
       printf ("aguardando sinais...\n");
       sleep_ms(2000);
   }
 }
