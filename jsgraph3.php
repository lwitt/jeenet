<html>
<script type="text/javascript" src="http://www.google.com/jsapi"></script> 
<script type="text/javascript"> 
google.load('visualization', '1', {packages: ['corechart']}); 
google.load('visualization', '1', {packages: ['gauge']});
</script> 

<?php
try
{
$bdd = new PDO('mysql:host=localhost;dbname=jeenet', 'pi', 'mdpmdp');

?>


<script type="text/javascript"> 

	var graphdate='2012-12-17';

	function drawVisualization() { 
		var data = google.visualization.arrayToDataTable([ ['x', 'puls/sec'], 

		<?php
			$date=$_GET['date'];
			if ($date=='')
				$date=date('Y-m-d');
			$reponse = $bdd->query('select id,time(date) as date,type,value,value2 from storage where type=\'W\' and date like \''.$date.'%\'');

			while ($donnees = $reponse->fetch())
				echo '[\''.$donnees['date'].'\','.$donnees['value'].'],';
			$reponse->closeCursor();
		?>
		]); 

		var options = {
          		title: 'Consommation maison <?php echo $date ?>',
          		width: 1000,
	   		height: 500,
	  		vAxis : {maxValue:120}
        	};		 

		new google.visualization.AreaChart(document.getElementById('visualization')).draw(data, options);


		gaugeOptions = { 
			min: 0, 
			max: 150, 
			greenFrom:0, greenTo: 50,
			yellowFrom: 50, yellowTo: 90, 
			redFrom: 90, redTo: 150, 
			minorTicks: 5 
		};

		gauge2Options = { 
			min: 0, 
			max: 40, 
			greenFrom:18, greenTo: 22,
			minorTicks: 1 
		};

		<?php
			$reponse = $bdd->query('select value,time(date) as time from storage where type=\'W\' order by date desc limit 1');
			$donnees = $reponse->fetch();
		?>
	
		var data2 = google.visualization.arrayToDataTable([ ['Label', 'Value'], ['puls/sec', <?php echo $donnees['value'];?>] ]); 

		<?php
			$reponse = $bdd->query('select value,time(date) as time from storage where type=\'H\' and node=1 order by date desc limit 1');
			$donnees = $reponse->fetch();
		?>
	
		var data3 = google.visualization.arrayToDataTable([ ['Label', 'Value'], ['Temp �C', <?php echo $donnees['value'];?>] ]);
		
		new google.visualization.Gauge(document.getElementById('gauge')). draw(data2,gaugeOptions);
		new google.visualization.Gauge(document.getElementById('gauge2')). draw(data3,gauge2Options);


		var data4 = google.visualization.arrayToDataTable([ ['x', '�C'], 

		<?php
			$date=$_GET['date'];
			if ($date=='')
				$date=date('Y-m-d');
			$reponse = $bdd->query('select id,time(date) as date,type,value,value2 from storage where type=\'H\' and node=1 and date like \''.$date.'%\'');

			while ($donnees = $reponse->fetch())
				echo '[\''.$donnees['date'].'\','.$donnees['value'].'],';
			$reponse->closeCursor();
		?>
		]); 

		var options2 = {
          		title: 'Temp�rature bureau <?php echo $date ?>',
          		width: 1000,
	   		height: 500,
	  		vAxis : {maxValue:35}
        	};		 

		new google.visualization.AreaChart(document.getElementById('visualization2')).draw(data4, options2);


	}
 
google.setOnLoadCallback(drawVisualization); 
</script> 
</head> 


<body style="font-family: Arial;border: 0 none;">
<table>
<tr><td><div id="gauge" style="width: 200px; height: 200px;"></div></td>
<td><div id="gauge2" style="width: 200px; height: 200px;"></div></td>
</tr></table>
<?php echo $donnees['time'];?> 
<div id="visualization" style="width: 1000px; height: 500px;"></div> 
<div id="visualization2" style="width: 1000px; height: 500px;"></div> 
<form action="<?php $_SERVER['PHP_SELF']; ?>">
	<select name="date" onchange="this.form.submit();">
		<?php 
			$reponse = $bdd->query('select distinct date(date) as date from storage order by date desc');
			while ($donnees = $reponse->fetch()) {
				echo '<option value=\''.$donnees['date'].'\'';
				if ($donnees['date']==$date) 
					echo ' selected';
				echo '>'.$donnees['date'].'</option>';
			}
			$reponse->closeCursor();
		?>
	</select>
</form>
<?php
}
catch (Exception $e)
{
        die('Erreur : ' . $e->getMessage());
}
?>

</body> 
</html>
