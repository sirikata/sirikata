<?php 

function head_dns($pieces) {
    $dns_curdir='/disk/local/meru/dns/global';

    $first = $pieces[3];
    if(strlen($first)>=2) {
        $dir1 = substr($first, 0, 2);
        if($dir1 == "." || $dir1 == "..")
            head_error('Invalid use of relative path');
        $dns_curdir .= "/$dir1";
        if(strlen($first)>=4) {
            $dir2 = substr($first, 2, 2);
            if($dir2 == "." || $dir2 == "..")
                head_error('Invalid use of relative path');
            $dns_curdir .= "/$dir2";
        }
    }

    for($cur=3; $cur<count($pieces)-1; $cur++) {
        if($pieces[$cur] == '.' || $pieces[$cur] == '..')
            head_error('Invalid use of relative path');
        $dns_curdir .= '/' . $pieces[$cur];
        if(!is_dir($dns_curdir))
            head_error('Directory does not exist', 404);
    }

    $dns_curdir .= '/' . $pieces[count($pieces)-1];
    if(!is_file($dns_curdir))
        head_error('File does not exist', 404);

    $contents = trim(file_get_contents($dns_curdir));

    $hash = substr($contents, 9);
    $dir1 = substr($hash, 0, 3);
    $dir2 = substr($hash, 3, 3);
    $files_dir = '/disk/local/meru/uploadsystem/files/global';
    $real_file = "$files_dir/$dir1/$dir2/$hash";
    $file_size = @filesize($real_file);
    if($file_size === NULL) $file_size = "Unknown";

    header('Accept-Ranges: bytes');
    header("Hash: $hash");
    header("File-Size: $file_size");
}

?>
