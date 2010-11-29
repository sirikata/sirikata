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

function head_error($msg, $code=400) {
  header("Meru-Error: $msg", true, $code);
  exit;
}

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