<html>
 <head>
  <title>Debug</title>
 </head>
 <body>
 <?php 
	try
	{
$bdd = new PDO('mysql:host=localhost;dbname=jeenet', 'pi', 'mdpmdp');

$reponse = $bdd->query('select id,name,checkin,lastalive,abilities from nodes');
echo 'nodes:<br><table border=1>';
while ($donnees = $reponse->fetch())
{
	echo '<tr><td>'.$donnees['id'] . '</td><td>' . $donnees['name'] . '</td><td>' . $donnees['checkin'] . '</td><td>' . $donnees ['lastalive'].'<td/><td>' . $donnees ['abilities'].'<td/></tr>';
}
echo '</table><br>';
$reponse->closeCursor();

$reponse = $bdd->query('select id,date,type,value,value2 from storage order by date desc limit 10');
echo 'storage:<br><table border=1>';
while ($donnees = $reponse->fetch())
{
	echo '<tr><td>'.$donnees['date'] . '</td><td>' . $donnees['type'] . '</td><td>' . $donnees['value'] . '</td><td>' . $donnees ['value2'].'<td/></tr>';
}
echo '</table>';
$reponse->closeCursor();

}
catch (Exception $e)
{
        die('Erreur : ' . $e->getMessage());
}
?>
 </body>
</html>
