if [istarget "pdp10-xkl-*"] {
    # Fails because width of long long isn't sizeof (long long) * CHAR_BIT.
    set torture_execute_xfail "pdp10-xkl-*"
}
return 0
