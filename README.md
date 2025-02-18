## Projeto de Controle de LEDs RGB e Conversao ADC com representacao grafica no Display SSD1306 com Raspberry Pi Pico W

## linkd do video explicatico: https://drive.google.com/file/d/1CPUYFKMaUaeDQEtSj8Wj5ekMxI-ww0Mz/view?usp=drivesdk

# Descrição Geral

Este projeto implementa o controle de LEDs RGB via PWM, a leitura de entradas analógicas de um joystick e a exibição de um quadrado em um display OLED SSD1306, utilizando a Raspberry Pi Pico W e a PCB BitDogLab. A movimentação do quadrado é controlada pelo joystick, e o sistema também inclui funcionalidades adicionais como o acionamento de um modo bootloader e o controle de estado do PWM via botões.

Funcionalidades

Controle de LEDs RGB: Utiliza PWM para ajustar a intensidade dos LEDs Azul e Vermelho com base nas entradas do joystick.

Movimentação do Quadrado no Display: O quadrado de 8x8 pixels é exibido no centro do display SSD1306 e se movimenta proporcionalmente ao deslocamento do joystick.

# Interrupções por Botões:

Botão do Joystick: Ativa uma animação de aumento de borda no display e muda o estado do led verde.

Botão A: Liga e desliga o controle PWM dos LEDs.

Botão B: Entra no modo bootloader.

Tratamento de Debouncing: Implementado para evitar leituras falsas dos botões.

 # Resolução de Bugs

Correção da Centralização do Quadrado: Ajustado o cálculo da posição inicial e o deslocamento para garantir que o quadrado permaneça centralizado.

Animação de Borda: Implementada uma animação sem o uso de alarmes para evitar travamentos.

Debouncing Independente: Criadas variáveis de tempo distintas para cada botão, prevenindo conflitos.

# Estrutura do Código

setup_pwm: Configuração inicial dos pinos de PWM.

setup_led_and_button: Inicialização dos LEDs e botões.

gpio_irq_handler: Handler de interrupções para os botões, contendo lógicas de controle e animação.

main: Loop principal que realiza a leitura dos valores do joystick, aplica o PWM nos LEDs e atualiza o display.
