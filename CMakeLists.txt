SET(SOURCES ui_standbySCREEN.c
    ui_totalpowerSCREEN.c
    ui_prot1SCREEN.c
    ui_port1SCREEN12.c
    ui_prot2SCREEN.c
    ui_prot3SCREEN.c
    ui_prot4SCREEN.c
    ui_port2SCREEN22.c
    ui_port3SCREEN32.c
    ui_port4SCREEN42.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_52731097.c
    ui_img_411710101.c
    ui_img_334752297.c
    ui_img_888914300.c
    ui_img_919862230.c
    ui_img_934861631.c
    fonts/ui_font_daoli24zhong.c
    fonts/ui_font_daoli26zhong.c
    fonts/ui_font_DaoLit36zhong.c
    fonts/ui_font_DaoLit36zhong3.c
    fonts/ui_font_DaoLiti30.c
    fonts/ui_font_DaoLiti36.c
    fonts/ui_font_DaoLiti72.c)

add_library(ui ${SOURCES})
