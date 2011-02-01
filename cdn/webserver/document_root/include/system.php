<?php 

function head_error($msg, $code=400) {
    header("Meru-Error: $msg", true, $code);
    exit;
}

?>
