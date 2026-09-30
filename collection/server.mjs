import http from 'node:http';
import { readFile, stat } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const root = path.resolve(fileURLToPath(new URL('./dist', import.meta.url)));
const port = Number(process.env.PORT);
if (!port) throw new Error('Start with make preview through Portless.');
const types = {'.html':'text/html; charset=utf-8','.css':'text/css','.js':'text/javascript','.wasm':'application/wasm','.png':'image/png','.jpg':'image/jpeg','.webp':'image/webp','.gbc':'application/octet-stream','.zip':'application/zip'};
http.createServer(async(req,res)=>{
 try {
  const url = new URL(req.url,'http://local');
  let file = path.resolve(root,'.'+decodeURIComponent(url.pathname));
  if(file!==root&&!file.startsWith(root+path.sep)){res.writeHead(403).end();return;}
  if((await stat(file)).isDirectory())file=path.join(file,'index.html');
  const data=await readFile(file);
  res.writeHead(200,{'Content-Type':types[path.extname(file)]||'application/octet-stream','Cache-Control':'no-store'});res.end(data);
 } catch {res.writeHead(404).end('Not found');}
}).listen(port,'127.0.0.1',()=>console.log('Chromatic collection ready on Portless port '+port));
