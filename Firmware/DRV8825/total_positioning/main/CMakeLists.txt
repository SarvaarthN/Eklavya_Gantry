file(GLOB SOURCES "../../src/*.c")
idf_component_register(SRCS "test.c"
                            ${SOURCES}
                    INCLUDE_DIRS "../../include"
                    REQUIRES esp_common driver)
