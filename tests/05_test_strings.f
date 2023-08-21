
: esc_color dup "\027[38;5;" swap + "m" + swap + . ;


0
begin
    dup
    esc_color

    ++ dup

    256 =
until

drop

"\027[0;0m" .cr
