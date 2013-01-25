<html>
<script type="text/javascript" src="http://www.google.com/jsapi"></script> 
<script type="text/javascript"> 
google.load('visualization', '1', {packages: ['corechart']}); 
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
			$reponse = $bdd->query('select id,time(date) as date,type,value,value2 from storage where date like \''.$date.'%\'');

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

//		new google.visualization.ColumnChart(document.getElementById('visualization')).draw(data, options);
		new google.visualization.AreaChart(document.getElementById('visualization')).draw(data, options);


//		new google.visualization.LineChart(document.getElementById('visualization')).draw(data, {curveType: "function", width: 1000, height: 500, vAxis: {maxValue: 10}} ); 
	}
 
google.setOnLoadCallback(drawVisualization); 
</script> 
</head> 


<body style="font-family: Arial;border: 0 none;"> 
<div id="visualization" style="width: 1000px; height: 500px;">
</div> 
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
