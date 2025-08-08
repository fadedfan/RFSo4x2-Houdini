#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <linux/input.h>

#define LED_BRIGHTNESS    "/sys/class/leds/led-pl1/brightness"
#define    LED_TRIGGER    "/sys/class/leds/led-pl1/trigger"
#define    INPUT_EVENT    "/dev/input/event1"
#define    LED_MAX_SPEED    10
#define    PERIOD_COEFF    10000

unsigned int led_speed;
pthread_mutex_t lock;

/* Blink LED */
static void *LEDMod(void *dummy)
{
    unsigned int led_period;
    int tmp;
    tmp = open(LED_BRIGHTNESS, O_WRONLY);
    if (tmp < 0)
        exit(1);
    while (1) {
        pthread_mutex_lock(&lock);
        led_period = (LED_MAX_SPEED - led_speed) * PERIOD_COEFF;
        pthread_mutex_unlock(&lock);

        write(tmp, "1", 2);
        usleep(led_period);
        write(tmp, "0", 2);
        usleep(led_period);
    }
}

int main()
{
    pthread_t pth;
    struct input_event ev;
    int tmp;
    int key_code;
    int size = sizeof(ev);

    /* Configure LED */
    led_speed = 3;
    tmp = open(LED_TRIGGER, O_WRONLY);
    if (tmp < 0)
        return 1;
    if (write(tmp, "default-on", 10) != 10) {
        printf("Error writing trigger");
        return 1;
    }
    close(tmp);
    printf("Configured LED for use\n");

    /* Create thread */
    pthread_mutex_init(&lock, NULL);
    pthread_create(&pth, NULL, LEDMod, "Blinking LED...");

    /* Read event0 */
    tmp = open(INPUT_EVENT, O_RDONLY);
    if (tmp < 0) {
        printf("\nOpen " INPUT_EVENT " failed!\n");
        return 1;
    }
    /* Read and parse event, update global variable */
    while (1) {
        if (read(tmp, &ev, size) < size) {
            printf("\nReading from " INPUT_EVENT " failed!\n");
            return 1;
        }

        if (ev.value == 1 && ev.type == 1) {    /* Down press only */
            key_code = ev.code;
            if (key_code == KEY_DOWN) {    /* lower speed */
                /* Protect from concurrent read/write */
                pthread_mutex_lock(&lock);
                if (led_speed > 0)
                    led_speed -= 1;
                pthread_mutex_unlock(&lock);
            } else if (key_code == KEY_UP) {    /* raise speed */
                pthread_mutex_lock(&lock);
                if (led_speed < 9)
                    led_speed += 1;
                pthread_mutex_unlock(&lock);
            }
            printf("Speed: %i\n", led_speed);
            usleep(1000);
        }
    }
}
