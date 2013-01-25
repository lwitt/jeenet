<html>
  <head><title>google latitude</title></head>
<body>
  <?php
  $url = "https://latitude.google.com/latitude/apps/badge/api?user=7204587664094710976&type=json";
  		$data = utf8_encode(file_get_contents($url));
//      var_dump(json_decode($data));
	$json=json_decode($data,TRUE);
	$place= $json['features'][0]['properties']['reverseGeocode'];
	$longitude = $json["features"][0]["geometry"]["coordinates"][0];
	$latitude = $json["features"][0]["geometry"]["coordinates"][1];
	$timestamp = $json["features"][0]["properties"]["timeStamp"];
	
	$longitude_maison = 7.288264;
	$latitude_maison= 47.656974;

	echo $place."<br>";
	echo $longitude."<br>";
	echo $latitude."<br>";
	echo round((time()-$timestamp)/60)."<br>";
	
	$lat1 = deg2rad($latitude_maison);
	$long1 = deg2rad($longitude_maison);
	$lat2 = deg2rad($latitude);
	$long2 = deg2rad($longitude);

	$dp = distance($lat1,$long1,$lat2,$long2);
	
	echo "distance=".round($dp)."<br>";
	
	function distance($lat_a, $lon_a, $lat_b, $lon_b) { 
	$t1 = sin($lat_a) * sin($lat_b); 
	$t2 = cos($lat_a) * cos($lat_b); 
	$t3 = cos($lon_a-$lon_b); 
	
	$t4 = $t2 * $t3; 
	$t5 = $t1 + $t4; 
	$rad_dist = atan(-$t5/sqrt(-$t5 * $t5 +1)) + 2 * atan(1); 
	return ($rad_dist * 3437.74677 * 1.1508) * 1.6093470878864446; 
	}
?>

</body>
</html>
