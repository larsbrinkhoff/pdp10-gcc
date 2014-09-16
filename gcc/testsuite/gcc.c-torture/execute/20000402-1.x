# At -O0 this test calls _udivdi3 which doesn't work on the PDP-10.
set torture_eval_before_execute {
    set compiler_conditional_xfail_data {
        "_udivdi3 fails" \
        "pdp10-xkl-*" \
        { "-O0" } \
        { "" }
        }
}

return 0
