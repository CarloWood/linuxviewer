cmake_minimum_required(VERSION 3.10)

# Find the jekyll*.sh scripts.
set(JEKYLL_SERVE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/jekyll-serve.sh")
set(JEKYLL_CLEAN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/jekyll-clean.sh")

# Create a custom target for running the build-jekyll.sh script.
add_custom_target(jekyll_serve
    COMMAND ${JEKYLL_SERVE_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Jekyll serve..."
    USES_TERMINAL
)

add_custom_target(jekyll_clean
    COMMAND ${JEKYLL_CLEAN_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Jekyll clean..."
    USES_TERMINAL
)

add_dependencies(real-clean jekyll_clean)

# Make sure the script is executable.
file(CHMOD ${JEKYLL_SERVE_SCRIPT} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
