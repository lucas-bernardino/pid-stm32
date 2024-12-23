#include <stdint.h>
#include <stdlib.h>
#include "miros.h"
#include "pid.h"
#include "VL53L0X.h"
#include "config_gpio.h"
#include "stm32f1xx_hal.h"

float pwmVal = 0;
uint32_t previousTick = 0;
int currentDistance;

struct_tasks struct_distance_sensor_task;
struct_tasks struct_calc_pid;
struct_tasks struct_pwm_actuator_task;
struct_tasks struct_aperiodic_task;

OSThread_periodics_task_parameters parameters_distance_sensor_task;
OSThread_periodics_task_parameters parameters_calc_pid;
OSThread_periodics_task_parameters parameters_pwm_actuator_task;

PIDController pidController;
semaphore_t mutex_setpoint;
semaphore_t mutex_current_distance;
semaphore_t mutex_pwm_value;

TIM_HandleTypeDef htim2;

struct VL53L0X distanceSensor;

static struct VL53L0X myTOFsensor = {.io_2v8 = false, .address = 0x52, .io_timeout = 500, .did_timeout = false};

void read_distance_sensor();
void calc_PID();
void pwm_actuator();
void distance_sensor_init();
void MX_TIM2_Init(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

int main() {
    uint32_t stack_idleThread[40];

    OS_init(stack_idleThread, sizeof(stack_idleThread));

    MX_GPIO_Init();
    MX_TIM2_Init();
    distance_sensor_init();

    semaphore_init(&mutex_setpoint, 1, 1);
    semaphore_init(&mutex_current_distance, 1, 1);
    semaphore_init(&mutex_pwm_value, 1, 1);
    PID_setup(&pidController, -0.0001, -0.00001, -0.00001, 200, 0.3, -0.3);

    parameters_distance_sensor_task.deadline_absolute = 5;
    parameters_distance_sensor_task.deadline_dinamic = 5;
    parameters_distance_sensor_task.period_absolute = 5;
    parameters_distance_sensor_task.period_dinamic = 5;

    parameters_calc_pid.deadline_absolute = 5;
    parameters_calc_pid.deadline_dinamic = 5;
    parameters_calc_pid.period_absolute = 5;
    parameters_calc_pid.period_dinamic = 5;

    parameters_pwm_actuator_task.deadline_absolute = 5;
    parameters_pwm_actuator_task.deadline_dinamic = 5;
    parameters_pwm_actuator_task.period_absolute = 5;
    parameters_pwm_actuator_task.period_dinamic = 5;

    struct_distance_sensor_task.TCB_thread.task_parameters = &parameters_distance_sensor_task;
    struct_calc_pid.TCB_thread.task_parameters = &parameters_calc_pid;
    struct_pwm_actuator_task.TCB_thread.task_parameters = &parameters_pwm_actuator_task;

    OSPeriodic_task_start(&struct_distance_sensor_task.TCB_thread, 
                            &read_distance_sensor,
                            struct_distance_sensor_task.stack_thread,
                            sizeof(struct_distance_sensor_task.stack_thread));

    OSPeriodic_task_start(&struct_calc_pid.TCB_thread, 
                            &calc_PID,
                            struct_calc_pid.stack_thread,
                            sizeof(struct_calc_pid.stack_thread));
    
    OSPeriodic_task_start(&struct_pwm_actuator_task.TCB_thread, 
                            &pwm_actuator,
                            struct_pwm_actuator_task.stack_thread,
                            sizeof(struct_pwm_actuator_task.stack_thread));

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    OS_run();
}

void read_distance_sensor(){
    while(1){
        currentDistance = (int) VL53L0X_readRangeContinuousMillimeters(&distanceSensor);

        sem_down(&mutex_current_distance);
        pidController.input = currentDistance;
        sem_up(&mutex_current_distance);

        OS_wait_next_period();
    }
}

void calc_PID(){
    while(1){

        sem_down(&mutex_current_distance);
        sem_down(&mutex_setpoint);

        float error = pidController.setpoint - pidController.input;

        sem_up(&mutex_setpoint);
        sem_up(&mutex_current_distance);

        float pid_pwm_value = PID_action(&pidController, error);

        sem_down(&mutex_pwm_value);
        pwmVal = pid_pwm_value + 0.61;
        sem_up(&mutex_pwm_value);

        OS_wait_next_period();
    }
}

void pwm_actuator(){
    while(1){

        sem_down(&mutex_pwm_value);
        TIM2->CCR1 = (int) (pwmVal*TIM2->ARR);
        sem_up(&mutex_pwm_value);

        OS_wait_next_period();
    }
}

void aperiodic_task(){

    sem_down(&mutex_setpoint);

    if (pidController.setpoint == 400)
        pidController.setpoint = 200;
    else
        pidController.setpoint = 400;
    
    sem_up(&mutex_setpoint);

    OS_finished_aperiodic_task();
}

void distance_sensor_init() {

    while(!VL53L0X_init(&myTOFsensor));
    VL53L0X_setMeasurementTimingBudget(&myTOFsensor, 20e3); // 20 ms
    VL53L0X_startContinuous(&myTOFsensor, 0);
    
    return;
}

void MX_TIM2_Init(void){

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 500-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
	  OS_error();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
	  OS_error();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
	  OS_error();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
	  OS_error();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
	  OS_error();
  }
  HAL_TIM_MspPostInit(&htim2);
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

	uint32_t currentTick = HAL_GetTick();

	if (GPIO_Pin == GPIO_PIN_0 && (currentTick - previousTick) > 10){
		    OSAperiodic_task_start(&struct_aperiodic_task.TCB_thread,
                                    &aperiodic_task,
                                    struct_aperiodic_task.stack_thread,
                                    sizeof(struct_aperiodic_task.stack_thread));
		    previousTick = currentTick;
	}
}
