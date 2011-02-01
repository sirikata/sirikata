<?php

function head_file($pieces) {
    $files_dir = '/disk/local/meru/uploadsystem/files/global';
    $hash = $pieces[3];
    $dir1 = substr($hash, 0, 3);
    $dir2 = substr($hash, 3, 3);
    $file = "$files_dir/$dir1/$dir2/$hash";

    if(!is_file($file))
        head_error('File does not exist', 404);

    $file_size = @filesize($file);
    if($file_size === NULL)
        head_error('Error getting file size', 500);

    header('Accept-Ranges: bytes');
    header("Content-Length: $file_size");
}

?>
