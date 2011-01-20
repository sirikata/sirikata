<?php

require_once('include/validate.php');
require_once('include/system.php');

require_once('include/head_dns.php');
require_once('include/head_file.php');
require_once('include/get_file.php');

$method = $_SERVER['REQUEST_METHOD'];

if(array_key_exists('path', $_GET)) {
    $path = $_GET['path'];
} else {
    $path = '';
}

$pieces = explode('/', $path);

if($method == 'HEAD' &&
    count($pieces) >=3 &&
    $pieces[0] == '' &&
    $pieces[1] == 'dns' &&
    $pieces[2] == 'global') {

    head_dns($pieces);

} else if($method == 'HEAD' &&
    count($pieces) == 4 &&
    $pieces[0] == '' &&
    $pieces[1] == 'files' &&
    $pieces[2] == 'global' &&
    validate_sha256($pieces[3])) {

    head_file($pieces);

} else if($method == 'GET' &&
    count($pieces) == 4 &&
    $pieces[0] == '' &&
    $pieces[1] == 'files' &&
    $pieces[2] == 'global' &&
    validate_sha256($pieces[3])) {

    get_file($pieces);

} else {
    head_error('Invalid Path', 404);
}

?>