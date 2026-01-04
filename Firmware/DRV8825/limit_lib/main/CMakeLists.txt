idf_component_register(SRCS     "limit_lib.c"
                                "../../src/limit.c"
                   INCLUDE_DIRS "../../include"
                                
                  REQUIRES esp_common driver)

# set(COMPONENT_REQUIRES )
# set(COMPONENT_PRIV_REQUIRES )

# set(COMPONENT_SRCS "limit_lib.c")
# set(COMPONENT_ADD_INCLUDEDIRS ".")

# register_component()