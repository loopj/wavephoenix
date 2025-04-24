function(add_ota_flash_target TARGET)
  set(BINARY "${TARGET}.gbl")

  add_custom_target(
    "${TARGET}_flash_ota"
    COMMAND
      wavephoenix flash ${BINARY}
    DEPENDS ${TARGET}
  )
endfunction()
