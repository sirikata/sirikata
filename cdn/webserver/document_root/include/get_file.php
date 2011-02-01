<?php 

function get_file($pieces) {
    $files_dir = '/disk/local/meru/uploadsystem/files/global';
    $hash = $pieces[3];
    $dir1= substr($hash,0, 3);
    $dir2= substr($hash,3, 3);
    $file = "$files_dir/$dir1/$dir2/$hash";

    if(!is_file($file))
        head_error('File does not exist', 404);

    $file_size = @filesize($file);
    if($file_size === NULL)
        head_error('Error getting file size', 500);

    header('Accept-Ranges: bytes');
    if(array_key_exists('HTTP_RANGE', $_SERVER)) {
        $range = $_SERVER['HTTP_RANGE'];

        $num = preg_match('/bytes=(\d+)-(\d+)?/', $range, $matches);
        if($num == 0)
            head_error('Invalid Range');

        $offset = intval($matches[1]);
        $endnum = intval($matches[2]);
        $length = $endnum - $offset + 1;

        if($offset > $file_size || $endnum > $file_size)
            head_error('Invalid Range');
    } else {
        $offset = 0;
        $length = $file_size;
    }

    if($length < $file_size)
        header("Content-Range: bytes $offset-$endnum/$file_size");

    $fullbuff = "";

    $f = fopen($file, 'r');
    fseek($f, $offset);
    $numread = 0;
    while($numread < $length && !feof($f)) {
        $to_read = min(8192, $length-$numread);
        $data = @fread($f, $to_read);
        if($data === FALSE)
            exit;
        $fullbuff .= $data;
        $numread += $to_read;
    }

    if($numread < $length)
        exit;

    if(array_key_exists('HTTP_ACCEPT_ENCODING', $_SERVER) &&
        substr_count($_SERVER['HTTP_ACCEPT_ENCODING'], 'gzip') > 0) {
            
        header("Content-Encoding: gzip");
        $compressed_buff = gzencode($fullbuff);
        $compressed_len = strlen($compressed_buff);
        header("Content-Length: $compressed_len");
        echo $compressed_buff;
    } else {
        header("Content-Length: $length");
        echo $fullbuff;
    }

}

?>
