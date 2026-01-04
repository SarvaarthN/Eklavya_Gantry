cmake_minimum_required(VERSION 3.22)

set(srcs "src/limit.c" "src/drv8825.c" "src/combined.c")
set(include_dirs include)



idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES driver efuse esp_adc esp_common esp_event 
                                esp_rom esp_timer freertos heap log soc spiffs)