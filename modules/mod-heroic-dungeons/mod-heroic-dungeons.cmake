target_include_directories(modules PRIVATE "${CMAKE_SOURCE_DIR}/deps/fkYAML")

install(FILES
    "${CMAKE_SOURCE_DIR}/modules/mod-heroic-dungeons/conf/heroic_dungeons.yaml.dist"
    DESTINATION "${CONF_DIR}/modules")
