idf_component_register(
        SRC_DIRS "src" "src/internal"
        INCLUDE_DIRS "src"
        REQUIRES driver ESP32-N64-RMT
        PRIV_REQUIRES esp_timer)