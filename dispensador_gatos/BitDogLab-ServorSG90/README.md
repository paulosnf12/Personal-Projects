# Servo Motor - SG90

[Datasheet de referência](https://www.friendlywire.com/projects/ne555-servo-safe/SG90-datasheet.pdf)

## Módulo de Controle para Servo Motor SG90

Esta documentação detalha o funcionamento e a aplicação de um módulo de software para controlar o servo motor SG90 com o Raspberry Pi Pico.

## 1. Como Utilizar o Módulo

### 1.1. Conexão de Hardware

| Componente | Pino no Pico | Observação |
| --- | --- | --- |
| **Servo VCC (vermelho)** | **VBUS**  | **Recomendado.** Fornece 5V da porta USB. Para múltiplos servos, use uma fonte externa. |
| **Servo GND (marrom)** | **GND**  | **Essencial.** O terra (GND) deve ser comum entre o Pico e o servo. |
| **Servo Sinal (laranja)** | **Qualquer pino GPIO** | No nosso módulo, o padrão é **GPIO0 (SDA - I2C 0)**, mas pode ser alterado no arquivo `servo.h`. |

>**Ponto Importante:**  Alimentação inadequada  pode causar falhas. Se o servo apresentar comportamento instável, utilize uma   **fonte de 5V**, garantindo que o GND da fonte esteja conectado a um pino GND do Pico.

> Para este projeto, conectamos o servo motor à placa BitDogLab através do conector rotulado `I2C 0`. É importante notar que, apesar do nome do conector, não utilizamos o protocolo de comunicação I2C. O servo requer apenas um sinal PWM (Modulação por Largura de Pulso), que pode ser gerado por um pino GPIO comum. Portanto, em nosso software, configuramos o pino GPIO correspondente à linha SDA desta porta para operar como uma saída PWM, controlando diretamente o motor.

### 1.2. Integração dos Arquivos

1. `servo.h` para o diretório de cabeçalhos do seu projeto (ex: `inc/`).
2. `servo.c` para o diretório de código-fonte do seu projeto (ex: `src/`).

### 1.3. Configuração do Build (`CMakeLists.txt`)

Para que o SDK do Pico compile seu projeto corretamente, adicione `servo.c` às fontes e `hardware_pwm` às bibliotecas.

```makefile
# Adicione servo.c à lista de executáveis
add_executable(meu_projeto
    main.c
    src/servo.c
    # ... outros arquivos .c
)

# Adicione a biblioteca hardware_pwm
target_link_libraries(meu_projeto 
 pico_stdlib 
 hardware_pwm
)
```

### 1.4. Exemplo de Uso no Código

Com os arquivos integrados e o build configurado, usar o servo é simples:

```c
#include "pico/stdlib.h"
#include "servo.h" // Inclui a interface do nosso módulo

int main() {
    // Inicializações do seu sistema (ex: stdio_init_all())
    stdio_init_all();
    sleep_ms(1000);

    // 1. Inicializa o hardware PWM para o servo uma única vez
    servo_init();

    // 2. Mova o servo para a posição desejada (0-180 graus)
    servo_set_angle(90); // Move para a posição central

    while(1) {
        // A lógica do seu programa pode continuar aqui.
        // O servo permanecerá na última posição definida.
    }

    return 0;
}
```

## 2. Análise Técnica do Módulo de Servo

### 2.1. Princípios de Controle do Servo (PWM)

O servo SG90 é controlado por um sinal de Modulação por Largura de Pulso (PWM) com as seguintes características:

- **Frequência:** 50 Hz (um ciclo completo a cada 20 ms).
- **Largura do Pulso:** A duração do pulso em nível alto dentro desses 20 ms determina o ângulo.
  - **~500 µs (0.5 ms):** Posição de 0°.
  - **~2500 µs (2.5 ms):** Posição de 180°.

### 2.2. Arquivo de Cabeçalho `servo.h`

```c
#ifndef SERVOR_H
#define SERVOR_H

#include <stdio.h> // Para a função printf
#include "hardware/pwm.h" // Para todas as funções pwm_*
#include "hardware/clocks.h" // Para clock_get_hz e clk_sys
#include "hardware/gpio.h" // Para gpio_set_function e GPIO_FUNC_PWM

// --- Constantes para o controle do Servo ---
// Frequência do sinal PWM para servos (geralmente 50Hz, ou um período de 20ms)
#define PWM_PERIOD_MS 20

// Define o pino GPIO que está conectado ao fio de sinal do servo
// como está conectado ao I2C 0 e o seu canal SDA está conectado ao pino 0
#define SERVO_PIN 0

// Largura do pulso em microssegundos (us) para o SG90
#define SERVO_MIN_PULSE_US 500 // Pulso para o ângulo 0 no nosso código
#define SERVO_MAX_PULSE_US 2500 // Pulso para o ângulo 180 no nosso código

// Function prototypes
void servo_init();
void servo_set_angle(uint angle);

#endif // SERVOR_H
```

Este arquivo é a **interface pública** do módulo. Ele define:

- `SERVO_PIN`: O pino GPIO ao qual o servo está conectado.
- `SERVO_MIN_PULSE_US` e `SERVO_MAX_PULSE_US`: As larguras de pulso mínima e máxima, calibradas para o servo, ou seja, definirá o ponto mínimo e máximo ao qual o servo irá rotacionar. Estes são os valores mais importantes para ajustar caso um servo diferente seja utilizado.
- Protótipos das funções `servo_init()` e `servo_set_angle()`.

### 2.3. Implementação `servo.c`

```c
#include "servo.h"

// Variáveis estáticas para guardar a configuração do PWM
static uint slice_num;
static uint channel_num;
static uint32_t wrap_value;

// Função auxiliar para converter ângulo em largura de pulso
static uint32_t angle_to_pulse_width(uint angle) {
    // Mapeia o ângulo (0-180) para a faixa de pulso (MIN-MAX)
    uint32_t pulse = (angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / 180) + SERVO_MIN_PULSE_US;
    return pulse;
}

void servo_init() {
    printf("Configurando Servo SG90 no pino GPIO %d...\n", SERVO_PIN);

    // 1. Associa o pino GPIO à função de hardware PWM
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);

    // 2. Descobre qual "fatia" (slice) e "canal" de PWM o pino usa
    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    channel_num = pwm_gpio_to_channel(SERVO_PIN);

    // 3. Configura o PWM
    pwm_config config = pwm_get_default_config();
    
    // Calcula o divisor do clock para ter um período de 20ms (50Hz)
    // Para simplificar, vamos fazer cada ciclo do PWM contar 1 microssegundo (us)
    float div = (float)clock_get_hz(clk_sys) / 1000000;
    pwm_config_set_clkdiv(&config, div);
    
    // Define o valor de "wrap" (o período total) para 20.000us = 20ms
    wrap_value = PWM_PERIOD_MS * 1000;
    pwm_config_set_wrap(&config, wrap_value);

    // 4. Carrega a configuração na fatia PWM e a ativa
    pwm_init(slice_num, &config, true);
    
    printf("Servo SG90 pronto.\n");
}

void servo_set_angle(uint angle) {
    if (angle > 180) {
        angle = 180; // Limita o ângulo a 180
    }

    // Converte o ângulo para a largura do pulso e define o nível do canal PWM
    uint32_t pulse = angle_to_pulse_width(angle);
    pwm_set_chan_level(slice_num, channel_num, pulse);
}
```

- **`servo_init()`**: Esta função configura o periférico de hardware PWM do Pico.
    1. **Define a função do pino:** `gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM)` instrui o Pico a entregar o controle do pino para o hardware PWM.
    2. **Configura o Clock:** O clock do sistema (125 MHz) é dividido para que o contador do PWM incremente a cada 1 microssegundo. Isso torna os cálculos subsequentes mais intuitivos.
    3. **Define o Período (Wrap):** O contador é configurado para "zerar" (wrap) ao atingir 20.000. Como cada contagem leva 1 µs, o período total é de 20.000 µs (ou 20 ms), resultando na frequência de 50 Hz.
    4. **Ativa o PWM:** A configuração é carregada e o slice PWM é ativado.
- **`servo_set_angle(uint angle)`**: Esta função comanda o movimento.
    1. **Mapeamento Linear:** A função privada `angle_to_pulse_width()` converte o ângulo (0-180) para o valor de pulso correspondente na faixa calibrada (500-2500 µs).
    2. **`pwm_set_chan_level()`**: Esta é a função principal. Ela define a "largura do pulso", ou seja, o valor que o contador deve atingir antes de baixar o sinal para o nível lógico zero. Ao definir este nível, a posição do servo é efetivamente alterada.    

> Documentação por [@Nicolas Rafael](https://github.com/NicolasRaf)
