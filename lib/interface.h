#ifndef INTERFACE_H
#define INTERFACE_H

const char HTML_BODY[] =
"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
"<title>Estação Meteorológica</title>"
"<style>"
"body{font-family:Arial;text-align:center}"
"canvas{background:#fff;border:1px solid #ccc}"
".pot{font-size:1.2em;margin:5px}"
"</style>"

"<script>"
"let historico=[];"
"let paginaAtual=null;"
"let camposPreenchidos=false;"

"function atualizar(){"
" fetch('/estado')"
" .then(res=>res.json())"
" .then(data=>{"

" document.getElementById('nome').innerText=data.nome;"
" document.getElementById('valor').innerText="
"   data.valor.toFixed(2)+' '+(data.unidade||'');"

" document.getElementById('pot1').innerText="
"   data.pot1.toFixed(1)+' %';"

" document.getElementById('pot2').innerText="
"   data.pot2.toFixed(1)+' %';"

" historico=data.historico;"
" desenharGrafico();"

" if(paginaAtual!==data.pagina||!camposPreenchidos){"
"  lim_min.value=data.lim_min;"
"  lim_max.value=data.lim_max;"
"  offset.value=data.offset;"
"  paginaAtual=data.pagina;"
"  camposPreenchidos=true;"
" }"
" });"
"}"

"function desenharGrafico(){"
" let c=grafico;"
" let ctx=c.getContext('2d');"
" ctx.clearRect(0,0,c.width,c.height);"
" if(historico.length<2) return;"
" let min=Math.min(...historico),max=Math.max(...historico);"
" if(min==max){min--;max++;}"
" ctx.beginPath();"
" historico.forEach((v,i)=>{"
"  let x=i*(c.width/(historico.length-1));"
"  let y=c.height-(c.height*(v-min)/(max-min));"
"  i?ctx.lineTo(x,y):ctx.moveTo(x,y);"
" });"
" ctx.strokeStyle='#336699';ctx.lineWidth=2;ctx.stroke();"
"}"

"function enviarConfig(){"
" fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},"
" body:JSON.stringify({"
" pagina:paginaAtual,"
" lim_min:parseFloat(lim_min.value),"
" lim_max:parseFloat(lim_max.value),"
" offset:parseFloat(offset.value)"
" })});"
" return false;"
"}"

"function mudarPagina(){fetch('/pagina',{method:'POST'});}"

"setInterval(atualizar,1000);"
"window.onload=atualizar;"
"</script></head>"

"<body>"
"<h1>Estação Meteorológica</h1>"

"<h2 id='nome'>--</h2>"
"<div id='valor' style='font-size:2.5em'>--</div>"

"<canvas id='grafico' width='320' height='100'></canvas>"

"<h3>Potenciômetros</h3>"
"<div class='pot'>Pot 1: <span id='pot1'>--</span></div>"
"<div class='pot'>Pot 2: <span id='pot2'>--</span></div>"

"<form onsubmit='return enviarConfig()'>"
"Lim Min <input id='lim_min' type='number' step='any'><br>"
"Lim Max <input id='lim_max' type='number' step='any'><br>"
"Offset <input id='offset' type='number' step='any'><br>"
"<button>Salvar</button>"
"</form>"

"<button onclick='mudarPagina()'>Mudar Página</button>"
"</body></html>";

#endif
