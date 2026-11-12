idf_component_register(SRCS "main.c" "sx1262_driver.h" "sx1262_driver.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_gpio esp_timer
                    PRIV_REQUIRES esp_driver_spi
                    )
