void append_page_header() {
  webpage  = F("<!DOCTYPE html><html lang=\"es\">");
  webpage += F("<head>");
  webpage += F("<title>Speaker</title>");
  webpage += F("<link href=\"https://fonts.googleapis.com/css?family=Oxygen|Ubuntu&display=swap\" rel=\"stylesheet\">");
  webpage += F("<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>");
  webpage += F("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>");
  webpage += F("<style>");
  webpage += F("body{max-width:65%;margin:0 auto;font-family:'Oxygen', sans-serif;font-size:105%;text-align:center;color: #035b6e;background-color:#FFFFFF;}");
  webpage += F("ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#007993;font-size:1em;}");
  webpage += F("li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}");
  webpage += F("li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%; color: white;}");
  webpage += F("li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}");
  webpage += F("section {font-size:0.88em;}");
  webpage += F("h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#007993;}");
  webpage += F("h2{color:orange;font-size:1.0em;}");
  webpage += F("h3{font-size:0.8em;}");
  webpage += F("table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;}");
  webpage += F("th,td {font-family: 'Oxygen', sans-serif;border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;box-shadow: 0px 0px 3px #ccc, 0 10px 15px #eee inset;border-radius:6px;}");
  webpage += F("tr:nth-child(odd) {background-color:#eeeeee;}");
  webpage += F(".rcorners_n {border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}");
  webpage += F(".rcorners_m {border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}");
  webpage += F(".rcorners_w {border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}");
  webpage += F(".column{float:left;width:50%;height:45%;}");
  webpage += F(".row:after{content:'';display:table;clear:both;}");
  webpage += F("*{box-sizing:border-box;}");
  webpage += F("input{box-shadow: 0px 0px 3px #ccc, 0 10px 15px #eee inset;border-radius:6px;}");
  webpage += F("footer{background-color:#a2d8e4; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}");
  webpage += F("button{border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:150px;color:white;font-size:130%;}");
  webpage += F(".buttons {border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:150px;color:white;font-size:80%;}");
  webpage += F(".buttonsm{border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:100px; color:white;font-size:70%;}");
  webpage += F(".buttonm {border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:150px;color:white;font-size:70%;}");
  webpage += F(".buttonw {border-radius:0.5em;background:#007993;padding:0.3em 0.3em;width:150px;color:white;font-size:70%;}");
  webpage += F("a{font-size:75%;}");
  webpage += F("p{font-size:75%;}");
  webpage += F("</style></head><body><h1>Speaker</h1>");
}

void append_page_footer() {
  webpage += F("<ul>");
  //webpage += F("<li><a href='/asetup'>Red Móvil</a></li>");
  webpage += F("</ul>");
  webpage += "<footer>www.orbittas.com</footer>";
  webpage += F("</body></html>");
}
