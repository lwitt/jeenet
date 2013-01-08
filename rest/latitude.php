<html>
  <head><title>google latitude</title></head>
<body>
  <?php
  $url = "https://latitude.google.com/latitude/apps/badge/api?user=7204587664094710976&type=json";
  		$data = utf8_encode(file_get_contents($url));
      
      echo $data;
  ?>

</body>
</html>
