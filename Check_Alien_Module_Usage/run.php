<?php

if (!isset($argv[1]) ||empty($argv[1])){
  print "Missing module argument";
  exit(1);
}else{
  $moduleName=$argv[1];
}

$githubUSER = readline("Enter your github username: ");
$githubPWD = readline("Enter your github password: ");

$repos = [
  "ec-europa/ejtp-reference",
  "ec-europa/erxs",
  "ec-europa/irma-reference",
  "ec-europa/c4dweb-reference",
  "ec-europa/pics-reference",
  "ec-europa/ami-reference",
  "ec-europa/efsa-reference",
  "ec-europa/europahub-alien-dev",
  "ec-europa/roma-reference",
  "ec-europa/eac-eyp-reference",
  "ec-europa/fra-reference",
  "ec-europa/bbi-reference",
  "ec-europa/joinup-dev",
  "ec-europa/icdppc-reference",
];

foreach ($repos as $repo){
  $cmd="curl -s -u ${githubUSER}:${githubPWD} https://api.github.com/search/code?q=$moduleName+repo:$repo";
  $data = shell_exec($cmd);
  $dataJson = json_decode($data,true);
  print "----- $repo -----". PHP_EOL;
  if (isset($dataJson['message'])){
    print "ERROR: " . $dataJson['message'] . PHP_EOL;
  }elseif($dataJson['total_count'] === 0){
    print "`${moduleName}` NOT FOUND". PHP_EOL;
  }else{
    print "`${moduleName}` FOUND ON:" . PHP_EOL;
    foreach ($dataJson['items'] as $item){
      print "- " . $item['name'] . " => " . $item['html_url']. PHP_EOL;
    }
  }
}
