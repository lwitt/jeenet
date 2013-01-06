<?php

# error_reporting( E_ALL); 

error_reporting(E_ALL | E_STRICT );
  ini_set("display_errors", 1);


$weather_conditions=array(
  0=>"TORNADE",
  1=>"TEMPETE TROP",
  2=>"HURRICANE",
  3=>"ORAGES SEV.",
  4=>"ORAGES",
  5=>"NEIGE PLUIE",
  6=>"NEIGE FONDUE",
  7=>"NEIGE FONDUE",
  8=>"BRUINE VERGL",
  9=>"BRUINE",
  10=>"PLUIE VERGL",
  11=>"AVERSES",
  12=>"AVERSES",
  13=>"AVER. NEIGE",
  14=>"LEG NEIGE",
  15=>"POUDREUSE",
  16=>"NEIGE",
  17=>"GRELE",
  18=>"NEIGE FOND.",
  19=>"POUSSIERE",
  20=>"BRUMEUX",
  21=>"BRUME",
  22=>"ENFUME",
  23=>"TEMPETUEUX",
  24=>"VENTEUX",
  25=>"FROID",
  26=>"NUAGEUX",
  27=>"MAJ NUAGEUX",
  28=>"MAJ NUAGEUX",
  29=>"PAR NUAGEUX",
  30=>"PAR NUAGEUX",
  31=>"NUIT CLAIRE",
  32=>"ENSOLEILLE",
  33=>"NUIT CALME",
  34=>"JOUR CALME",
  35=>"PLUIE GRELE",
  36=>"CHAUD",
  37=>"ORAGE ISOLE",
  38=>"ORAGE EPARS",
  39=>"ORAGE EPARS",
  40=>"PLUIES EPAR",
  41=>"FORTE NEIGE",
  42=>"NEIGE EPAR",
  43=>"FORTE NEIGE",
  44=>"PAR COUVERT",
  45=>"ORAGES",
  46=>"AVERS NEIGE",
  47=>"ORAGE ISOLE",
  3200=>"NA"
);

$week=array(
	"Mon"=>"LUN",
	"Tue"=>"MAR",
	"Wed"=>"MER",
	"Thu"=>"JEU",
	"Fri"=>"VEN",
	"Sat"=>"SAM",
	"Sun"=>"DIM"
);

try
{

if (isset($_GET['f'])) {
	$fonction=$_GET['f'];

	$bdd = new PDO('mysql:host=localhost;dbname=jeenet', 'pi', 'mdpmdp');


	switch ($fonction) {
		case "hello":
			echo "Hello world !";
			break;

		case "init":
			echo "init";
			$bdd->exec('DELETE FROM nodes');
			break;

		case "addnode":
			echo "addnode";
			$value=$_GET['v'];

			if ($bdd->query('SELECT * FROM nodes WHERE id=\''.$value.'\'')->fetch()) {
				echo "already exists";
				if ($bdd->exec('UPDATE nodes SET lastalive=\''.date('Y-m-d H:i:s').'\'')>0)
						echo "ok";
					else
						echo "ko";

			}
			else {

				$req = $bdd->prepare('INSERT INTO nodes(id,checkin) VALUES(:id,:checkin)');

				if ($req->execute(array(
					'id' => $value,
					'checkin' => date('Y-m-d H:i:s')		
				)))
					echo "ok";
				else	
					echo "ko";
			} 
				

			break;

		case "removenode":
			echo "removenode";
			$value=$_GET['v'];

			if ($bdd->query('SELECT * FROM nodes WHERE id=\''.$value.'\'')->fetch()) {
				if ($bdd->exec('DELETE FROM nodes WHERE id=\''.$value.'\'')>0)
						echo "ok";
					else
						echo "ko";

			}
			else {
				echo 'node doesn\'t exist';
			}			 
				

			break;

		case "time":
			echo 'TIME:'.date('H:i:s:d:m:y');
			break;

		case "meteo":
			$url = "http://xml.weather.yahoo.com/forecastrss?p=FRXX0071&u=c";
			$data = utf8_encode(file_get_contents($url));
			$xml = simplexml_load_string($data);


			if($xml) {
				echo "ok";

				$channel_yweather = $xml->channel->children("http://xml.weather.yahoo.com/ns/rss/1.0");

				foreach($channel_yweather as $x => $channel_item) 
					foreach($channel_item->attributes() as $k => $attr) {
						if ($k=='city') $city=strtoupper($attr);
						if ($k=='humidity') $humidity=$attr;
						if ($k=='pressure') $pressure=$attr;
						if ($k=='sunrise') $sunrise=$attr;
						if ($k=='sunset') $sunset=$attr;
						//echo $k."=".$attr.strlen($k);
					}

				$item_yweather = $xml->channel->item->children("http://xml.weather.yahoo.com/ns/rss/1.0");
				
					
				foreach($item_yweather as $x => $yw_item) {
					foreach($yw_item->attributes() as $k => $attr) {
						if ($k=='code') $conditions_code[]=$attr; 
						if ($k=='temp') $temp=$attr; 
						if ($k=='low') $temp_low[]=$attr; 
						if ($k=='high') $temp_high[]=$attr; 
						if ($k=='day') $days[]=$attr; 
						//echo $k."=".$attr." ";
					}
				}
				
				echo "METEO:".$city.":".$weather_conditions[(string)$conditions_code[0]].":".$temp;
			}
			else
				echo "ko"; 
			break;
			

		case "store":
			if (isset($_GET['t']) AND isset($_GET['v'])) {
				$type=$_GET['t'];
				$node=$_GET['n'];
				$value=$_GET['v'];
 				$value2=$_GET['v2'];
	
				switch ($type) {
					case 'w':
					echo "Storing elec" . $value;

					$req = $bdd->prepare('INSERT INTO storage(node,type,date,value) VALUES(:node,:type,:date,:value)');
					if ($req->execute(array(
					'node' => $node,
					'type' => 'W',
					'date' => date('Y-m-d H:i:s'),
					'value' => $value		
					)))
						echo "ok";
					else
						echo "ko";
					break;
				
                                   case 'h':
                                   echo "Storing temp&humi" . $value;
					
					$reponse=$bdd->query('SELECT node,value,value2 FROM storage WHERE node='.$node.' ORDER BY date DESC LIMIT 1');
					
					if ($donnees=$reponse->fetch())
					{
						if ($donnees['value']!=$value && $donnees['value2']!=$value2) {
							$req = $bdd->prepare('INSERT INTO storage(node,type,date,value,value2) VALUES(:node,:type,:date,:value,:value2)');
							if ($req->execute(array(
								'node' => $node,
								'type' => 'H',
								'date' => date('Y-m-d H:i:s'),
								'value' => $value,
								'value2' => $value2		
								)))
       	                                  		echo "ok";
              	                     	else
                     	                     	echo "ko";
						}
						else
							echo "pass";
					} 
					else {
						$req = $bdd->prepare('INSERT INTO storage(node,type,date,value,value2) VALUES(:node,:type,:date,:value,:value2)');
						if ($req->execute(array(
							'node' => $node,
							'type' => 'H',
							'date' => date('Y-m-d H:i:s'),
							'value' => $value,
							'value2' => $value2		
							)))
       	                    echo "ok";
              	        else
                     	    echo "ko";
					}
						
                    break;
				}
     			}
			break;

		case "list":
			

		default:
			echo "unknown function";
	}
}
}
catch (Exception $e)
{
        die('Erreur : ' . $e->getMessage());
}
?>
