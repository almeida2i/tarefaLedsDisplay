#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define NUM_PIXELS 25      // Quantidade de pixels da BitDogLab
#define WS2812_PIN 7       // GPIO onde a cadeia de pixels está conectada
#define LED_RGB_GREEN 11   // GPIO onde o LED verde do LED RGB está conectado
#define LED_RGB_BLUE 12    // GPIO onde o LED azul do LED RGB está conectado
#define Intens 41          // Luminosidade do dígito
#define BOTAO_A 5          // GPIO referente ao Botão A
#define BOTAO_B 6          // GPIO referente ao Botão B

/// Função para envia um pixel para o PIO
static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

// Função para juntar os pixels em uma única word
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

// Mapas para construir dígitos decimais na matriz de LEDs
uint8_t digitos[10][25] = {
    {// 0
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,0,1,0,
     0,1,0,1,0,
     0,1,1,1,0},
    {// 1
     0,0,0,1,0,
     0,0,1,1,0,
     0,0,0,1,0,
     0,0,0,1,0,
     0,0,0,1,0},
    {// 2
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0},
    {// 3
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0},
    {// 4
     0,1,0,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,0,0,1,0},
    {// 5
     0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0},
    {// 6
     0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0},
    {// 7
     0,1,1,1,0,
     0,1,0,1,0,
     0,0,0,1,0,
     0,0,0,1,0,
     0,0,0,1,0},
    {// 8
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0},
    {// 9
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0}
};

// Função para mostrar dígitos decimais na matriz de LEDs
void digit_show(PIO pio, uint sm, uint8_t digit)
{
    for(int i=24; i>19; i--)
        put_pixel(pio,sm,urgb_u32(0,0,Intens*digitos[digit][i]));
    for(int i=15; i<20; i++)
        put_pixel(pio,sm,urgb_u32(0,0,Intens*digitos[digit][i]));
    for(int i=14; i>9; i--)
        put_pixel(pio,sm,urgb_u32(0,0,Intens*digitos[digit][i]));
    for(int i=5; i<10; i++)
        put_pixel(pio,sm,urgb_u32(0,0,Intens*digitos[digit][i]));
    for(int i=4; i>-1; i--)
        put_pixel(pio,sm,urgb_u32(0,0,Intens*digitos[digit][i]));
}

ssd1306_t ssd; // Inicializa a estrutura do display

// Variáveis para controle dos LEDs verde e azul com PWM
bool green_led = 0;
bool blue_led = 0;

// Variáveis para debouncing dos Botões A e B
absolute_time_t next_time_1 = 0;
absolute_time_t next_time_2 = 0;

// Função de callback dos botôes
void gpio_irq_callback(uint gpio, uint32_t event_mask){
    bool cor = true;
    if(gpio==BOTAO_A && absolute_time_diff_us(next_time_1,get_absolute_time())>=0){// Capitura do botão A com debounce
        next_time_1 = delayed_by_ms(get_absolute_time(),1000);
        green_led = !green_led;
        pwm_set_gpio_level(LED_RGB_GREEN,green_led*0x0fff); // Atualiza o estado do LED verde
        ssd1306_fill(&ssd, false); // Limpa o display
        ssd1306_send_data(&ssd);
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
        ssd1306_draw_string(&ssd, "LED   VERDE", 8, 10); // Desenha uma string
        if(green_led)
            ssd1306_draw_string(&ssd, "ACESO", 20, 30); // Desenha uma string
        else
            ssd1306_draw_string(&ssd, "APAGADO", 20, 30); // Desenha uma string
        ssd1306_send_data(&ssd); // Atualiza o display
    }

    cor = true;
    if(gpio==BOTAO_B && absolute_time_diff_us(next_time_2,get_absolute_time())>=0){// Capitura do botão A com debounce
        next_time_2 = delayed_by_ms(get_absolute_time(),1000);
        blue_led = !blue_led;
        pwm_set_gpio_level(LED_RGB_BLUE,blue_led*0x1fff);
        ssd1306_fill(&ssd, false); // Limpa o display
        ssd1306_send_data(&ssd);
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
        ssd1306_draw_string(&ssd, "LED   AZUL", 8, 10); // Desenha uma string
        if(blue_led)
            ssd1306_draw_string(&ssd, "ACESO", 20, 30); // Desenha uma string
        else
            ssd1306_draw_string(&ssd, "APAGADO", 20, 30); // Desenha uma string
        ssd1306_send_data(&ssd); // Atualiza o display
    }
}


int main()
{
    // Variáveis para capitura de caracteres do teclado
    char character1, character2 = 0;

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    // Inicialização da comunicação serial
    stdio_init_all();

    // Inicialização do PIO
    PIO pio; uint sm; uint offset;
    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000.f);

    // Inicializa os Botões A e B
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A,GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B,GPIO_IN);
    gpio_pull_up(BOTAO_B);

    // Inicializa módulo PWM
    gpio_set_function(LED_RGB_GREEN, GPIO_FUNC_PWM);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_RGB_GREEN), 1);
    gpio_set_function(LED_RGB_BLUE, GPIO_FUNC_PWM);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_RGB_BLUE), 1);

    // Configuração das interrupções nos botões A e B
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true,gpio_irq_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true,gpio_irq_callback);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    bool cor = true;

    for(int i=0; i<NUM_PIXELS; i++)
        put_pixel(pio,sm,urgb_u32(0,0,0));

    while (true)
    {
        cor = !cor;

        scanf("%c",&character1);
        if(character1 >= 48 && character1 <= 57 || character1 >= 65 && character1 <= 90)
            character2 = character1;

        // Mostra, na matriz de LEDs, o dígito decimal digitado no teclado
        if(character2 >= 48 && character2 <= 57)
            digit_show(pio,sm,character2-48);
        else
            for(int i=0; i<NUM_PIXELS; i++)
                put_pixel(pio,sm,urgb_u32(0,0,0));

        // Mostra, no display, o caractere digitado no teclado
        ssd1306_fill(&ssd, !cor); // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
        ssd1306_draw_string(&ssd, "CARACTERE", 8, 10);
        ssd1306_draw_string(&ssd, "DIGITADO", 8, 25);
        ssd1306_draw_char(&ssd, character2, 8, 40);
        ssd1306_send_data(&ssd); // Atualiza o display

        sleep_ms(1000);
    }
    return 0;
}