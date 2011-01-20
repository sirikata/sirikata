<?php

/*
 * Validates a string that should be hex of length 64
 *
 * Matches: 3c0124942cd1072be05821da25746c16330d2eaa38b00f23dd7509cc423cfec8
 * Non-Matches: 3847cf | a | klmnop
 */
function validate_sha256($str) {
    $res = preg_match('/^([0-9a-fA-F]){64}$/', $str);
    if($res == 0) return FALSE;
    return TRUE;
}

?>
