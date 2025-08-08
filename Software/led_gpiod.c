#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *chipname = "gpiochip1";
    const char *linename = "LED3";
    struct gpiod_chip *chip;
    struct gpiod_line *ledPin3;

    // Open GPIO chip
    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("Open chip failed");
        return -1;
    }

    // Open GPIO line
    ledPin3 = gpiod_chip_find_line(chip, linename);
    if (!ledPin3) {
        fprintf(stderr, "Cannot find line with name: %s\n", linename);
        gpiod_chip_close(chip);
        return -1;
    }

    // Open GPIO line for output
    if (gpiod_line_request_output(ledPin3, "example1", 0) < 0) {
        perror("Request line as output failed");
        gpiod_chip_close(chip);
        return -1;
    }

    // Blink LED in a binary pattern
    for (int i=0; i<100; i++) {
        gpiod_line_set_value(ledPin3, (i & 1) != 0);
        usleep(100000);
    }

    // Release lines and chip
    gpiod_line_release(ledPin3);
    gpiod_chip_close(chip);
    return 0;
}
