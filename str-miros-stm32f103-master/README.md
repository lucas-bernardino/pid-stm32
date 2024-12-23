### Controle da altura de uma bolinha com um controlador PID em um RTOS utilizando STM32F103

[Objetivos](#objetivos)

[Rate Monotonic (RM)](#rate-monotonic-rm)

[Background Server (BS)](#background-server-bs)

[Nom-Preemptive Protocol (NPP)](#nom-preemptive-protocol-npp)

[Estrutura das Tarefas Periódicas](#estrutura-das-tarefas-periódicas)

[Tarefas Aperiódicas](#tarefas-aperiódicas)

[Funções auxiliares](#funções-auxiliares)

[Escalonabilidade das tarefas do sistema](#escalonabilidade-das-tarefas-do-sistema)

## Objetivos

Esse repositório contém a implementação para o trabalho final da disciplina de Sistemas de Tempo Real, onde foi requisitado o controle da altura de uma bolinha utilizando um controlador PID em um RTOS embarcado. Para isso, foi utilizado o MiROS como sistema de tempo real, porém com uma série de modificações para atender os requisitos, tais como a construção de um escalonador para tarefa periódicas, um escalonador para tarefas aperiódicas e um protocolo de recursos para acesso compartilhado. 

O microcontrolador utilizado no projeto foi o STM32F103. Para medir a altura da bolinha, foi utilizado o sensor de distância VL53L0X. O sensor de atuação é um ventilador modelo AUB0912VH-CX09 que funciona com base em um PWM.

Para as tarefas periódicas, foi utilizado o *Rate Monotonic* (RM) e para tarefas aperiódicas foi utilizado o *Background Server* (BS). Além disso, foi utilizado o *Nom-Preemptive Protocol* (NPP) como algoritmo de compartilhamento de recursos. 

## Rate Monotonic (RM)
O algoritmo Rate Monotonic é muito utilizado em sistemas operacionais de tempo real (RTOS) como escalonador para tarefas periódicas. Ele é baseado em prioridades estaticas, de modo que elas são definidas de modo que quanto maior o período de uma tarefa, menor será a sua prioridade. Assim, é garantido que tarefas que devem ser executadas em ciclos menores possuam maior prioridade no sistema.

## Background Server (BS)
Em um sistema como tarefas periódicas e aperiódicas, foi assumido que as tarefas periódicas respeitarão o escalonamento por RM. Para as tarefas aperódicas, utilizou-se o Background Server, que funciona de uma maneira relativamente simples: quando não há nenhuma tarefa periódica sendo executada, o escalonador deve executar a fila de tarefas aperiódicas. Ou seja, quando não há tarefas periódicas, as tarefas aperiódicas são escolhidas de modo que aquelas que chegaram primeiro possuem a maior prioridade na fila.

## Nom-Preemptive Protocol (NPP)
Em sistemas multitarefas, geralmente se trabalha com exclusão mutua, de modo que existem alguns protocolos para garantir a exclusão mutua dos recursos a serem compartilhados. Essa parte do código é chamada de seção crítica e deve ser protegida por semáforos. 

O Nom-Preemptive Protocol (NPP) aumenta a prioridade de uma tarefa quando ela entra em uma seção crítica, de modo que ela recebe a maior prioridade do sistema a fim de que ela não sofra preempção. A sua prioridade deve voltar ao normal após sair da seção crítica. 

## Estrutura das Tarefas Periódicas
As tarefas periódicas correspondem a:
  1.   Ler o sensor de distância (*read_distance_sensor*)
  2.   Calcular o output do controlador PID (*calc_PID*)
  3.   Atuar no sensor que controla a altura da bolinha por meio do PWM (*pwm_actuator*)
   
Todas essas tarefas possuem um perído de 5ms.    

A tarefa que utiliza o sensor de distância possui uma área crítica, na qual ao atualizar o input do controlador PID, é preciso guardá-la com semáforos, utilizando o *mutex_current_distance*. O input, atualizado na struct do controlador PID, será posteriormente utilizado para calcular o erro e efetuar os cálculos do controlador.

A tarefa que calcula o output do controlador também é protegida por semáforos, utilizando o *mutex_current_distance* e *mutex_setpoint* na área crítica onde é atualizado o erro (setpoint - input). Após isso, o cálculo padrão de um controlador PID é feito, se atentando para não ultrapassar os valores mínimos e máximos. O output do cálculo, atualizado na struct do controlador, é protegido novamente com o mútex *mutex_pwm_value*.    

A tarefa que atua com o pwm é relativamente simples, onde novamente há a proteção da área crítica por semáforo, utilizando o *mutex_pwm_value*.   


## Tarefas Aperiódicas

O sistema possui uma tarefa aperiódica, que corresponde à eventual leitura do botão (*aperiodic_task*). Ao pressioná-lo ocorre uma mudança do setpoint do controlador, intercalando-se entre os valores de 200 e 400 mm. Também é protegido a mudança do setpoint por um semáforo, o *mutex_setpoint*. 

## Funções auxiliares

Há a presença de algumas funções auxiliares no código, tais como a *distance_sensor_init*, responsável por inicializar o sensor de distância e a *MX_TIM2_Init*, responsável pelas inicializações do PWM

## Escalonabilidade das tarefas do sistema

Para realizar o teste de escalonabilidade, foi considerado o custo das tarefas com uma margem de segurança para garantir que o sistema fosse escalonável mesmo em uma situação mais crítica. A tabela a seguir exibe os custos e períodos de cada tarefa periódica, em milisegundos.

| Tarefa    | $C_{i}$ | $D_{i}$ | $T_{i}$ |
|--------|-------  |-------  |-----    |
| pwm_actuator      | 5       | 50      | 50      | 
| read_distance_sensor      | 10      | 50      | 50      |
| calc_PID       | 25      | 50      | 50      |

Pode-se aplicar o teste de escalonabilidade por RM: $\sum\frac{Ci}{Ti} < n(2^{1/n}-1)$.
Assim, $\frac{5}{50} +\frac{10}{50} +\frac{25}{50} + < 3(2^{1/3} - 1)$ $\Rightarrow 0.8 < 0.779$. Como a igualdade não é verdadeira, não se pode afirmar que o sistema é escalonável por RM com esse teste. 
Partindo para o teste hiperbólico, tem-se:  $\prod (Ui +1) < 2$
Assim, $(\frac{5}{50} + 1)(\frac{10}{50} + 1)(\frac{25}{50} + 1) < 2$ $\Rightarrow 1.98 < 2$. Logo, pode-se afirmar que o sistema é escalonável por RM, segundo o teste hiperbólico.

Além disso, como a utilização das tarefas não é máxima ($U_{sistema} = 0.8$) e a tarefa aperiódica possui um custo de aproximadamente 10 ms, considerando margens de segurança, tanto as tarefas periódicas quanto as aperiódicas são devidamente escalonáveis no sistema.

