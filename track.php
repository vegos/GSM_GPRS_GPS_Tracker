<?

    if (!empty($_GET['lat']) && !empty($_GET['lon']) && !empty($_GET['wra']) && !empty($_GET['sats']))
        
        {
        function getParameter($par, $default = null)
            {
            if (isset($_GET[$par]) && strlen($_GET[$par])) return $_GET[$par];
            elseif (isset($_POST[$par]) && strlen($_POST[$par])) 
                return $_POST[$par];
            else return $default;
            }

        $file = 'gps.txt';
        $lat = getParameter("lat");
        $lon = getParameter("lon");
        $wra = getParameter("wra");
        $sat = getParameter("sats");
        $speed = getParameter("speed");
        $course = getParameter("course");
        $altitude = getParameter("alt");
        $temperature = getParameter("temp");
        
        $person = $lat.",".$lon.",".$wra.",".$sat.",".$speed.",".$course.",".$altitude.",".$temperature."\n";
        
        echo "
            DATA:\n
            Latitude: ".$lat."\n
            Longitude: ".$lon."\n
            Time: ".$wra."\n
            Satellites: ".$sat."\n
            Speed: ".$speed."\n
            Course: ".$course."\n
            Altitude: ".$altitude."\n
            Temp: ".$temperature;

        if (!file_put_contents($file, $person, FILE_APPEND | LOCK_EX))
            echo "\n\t Error saving Data\n";
        else echo "\n\t Data Save\n";
    }
    else {

?>

<!DOCTYPE html>
<html>
    
<head>

    <!-- Load Jquery -->

    <script language="JavaScript" type="text/javascript" src="jquery-1.10.1.min.js"></script>

    <!-- Load Google Maps Api -->

    <!-- IMPORTANT: change the API v3 key -->

    <script type="text/javascript"
      src="https://maps.googleapis.com/maps/api/js?key=YOUR_GOOGLE_MAPS_v3_API_KEY_GOES_HERE&sensor=false">
    </script>
    
    <!-- Initialize Map and markers -->

    <script type="text/javascript">
        var myCenter=new google.maps.LatLng(37.950905,23.682902);   // Where to center the map.
        var marker;
        var map;
        var mapProp;

        function initialize()
        {
            mapProp = {
              center:myCenter,
              zoom:15,
              mapTypeId:google.maps.MapTypeId.ROADMAP              
              };
            mark();
            setInterval('mark()',30000);
        }


        function mark()
        {
            map=new google.maps.Map(document.getElementById("googleMap"),mapProp);
            LineCordinates = new Array();
            var file = "gps.txt";
            $.get(file, function(txt) { 
                var lines = txt.split("\n");
                for (var i=0;i<lines.length;i++)
                {
                    console.log(lines[i]);
                    var words=lines[i].split(",");
                    if ((words[0]!="")&&(words[1]!="")&&(words[0]!='0.0000000')&&(words[1]!='0.0000000'))
                    {
                       if (i==lines.length-2){
                       
                        marker=new google.maps.Marker({
                              position:new google.maps.LatLng(words[0],words[1]),
                              title: 'MAGLA'
                              });
                        marker.setMap(map);
                        }
                        map.setCenter(new google.maps.LatLng(words[0],words[1]));
                        document.getElementById('wra').innerHTML=words[2];
                        document.getElementById('sat').innerHTML=words[3];
                        document.getElementById('speed').innerHTML=words[4];
                        document.getElementById('course').innerHTML=words[5];                        
                        document.getElementById('altitude').innerHTML=words[6];
                        document.getElementById('temperature').innerHTML=words[7];                        
                        LineCordinates[i]=new google.maps.LatLng(words[0],words[1]);
                        infowindow = new google.maps.InfoWindow({
                                     content: words[2]+'<br>'+
                                              'Speed: '+words[4]+'<br>'+
                                              'Altitude: '+words[6]+'<br>'+
                                              'Temperature: '+words[7]+'&degC'
                        });
                        
                    }                    
                }
                marker.setAnimation(google.maps.Animation.DROP);
                path = new google.maps.Polyline({
                       path: LineCordinates,
                       strokeColor: '#FF0000',
                       strokeOpacity: 2,
                       strokeWeight: 3
                });
                  
                
                google.maps.event.addListener(marker, 'click', function() {
                    infowindow.open(map,marker);
                });
                
                path.setMap(map);
            });

        }
        
        google.maps.event.addDomListener(window, 'load', initialize);        
        
    </script>
</head>

<body>
    <?php
        echo '    

        <!-- Draw information table and Google Maps div -->

        <div>
            <center><br />
                <b>MAGLA GPS Tracking</b><br /><br />
                <div id="superior" style="width:800px;border:1px solid">
                    <table style="width:100%">
                        <tr align=center>
                            <td>Date/Time</td>
                            <td>Speed</td>
                            <td>Altitude</td>
                            <td>Course (&deg)</td>
                            <td>Temperature (&degC)</td>
                            <td>Satellites</td>
                        </tr>
                        <tr align=center>
                            <td id="wra">'. date("d/m/Y h:m") .'</td>
                            <td id="speed"></td>
                            <td id="altitude"></td>
                            <td id="course"></td>
                            <td id="temperature"></td>
                            <td id="sat"></td>
                        </tr>
                </table>
                </div>
                <br /><br />
                <div id="googleMap" style="width:800px;height:700px;"></div>
            </center>
        </div>';
    ?>
</body>
</html>

<? } ?>
