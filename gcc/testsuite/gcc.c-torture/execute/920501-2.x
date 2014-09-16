if [istarget "pdp10-xkl-*"] {
    # Fails because _udivdi3 and _umoddi3 doesn't work for the PDP-10 format.
    set torture_execute_xfail "pdp10-xkl-*"
}
return 0
