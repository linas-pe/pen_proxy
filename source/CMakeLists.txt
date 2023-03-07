add_executable(pen_proxy)

set(LOCAL_FILES
    client.c
    connector.c
    main.c
    write_buffer.c
    client.h
    connector.h
    write_buffer.h
)
target_sources(pen_proxy PRIVATE ${LOCAL_FILES})

install(TARGETS pen_proxy)

