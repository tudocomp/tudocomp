# website - Builds the website
add_custom_target(website
    COMMENT "Generating website" VERBATIM
    DEPENDS docs-main-html docs-doxygen
    # TODO copy and move everything so it's ready for the webserver
)

