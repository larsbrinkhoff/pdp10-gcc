if [istarget "pdp10-*-*"] {
    # Incomplete implementation of sprintf on the PDP-10.
    set torture_execute_xfail "pdp10-*-*"
}
return 0
