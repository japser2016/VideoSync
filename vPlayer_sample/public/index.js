// Import the Node.js http module

var http = require('http'); 
var fs = require("fs");
var path = require("path");
var url = require("url");
// req is the request object which is
// coming from the client side
// res is the response object which is going
// to client as response from the server

const mimeType = {
    '.ico': 'image/x-icon',
    '.html': 'text/html',
    '.js': 'text/javascript',
    '.json': 'application/json',
    '.css': 'text/css',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.wav': 'audio/wav',
    '.mp3': 'audio/mpeg',
    '.svg': 'image/svg+xml',
    '.pdf': 'application/pdf',
    '.doc': 'application/msword',
    '.eot': 'application/vnd.ms-fontobject',
    '.ttf': 'application/font-sfnt',
    '.mp4': 'video/mp4'
};

// Create a server object
var PORT = 8081

http.createServer( (req, res) => {
     
   
    // Parsing the requested URL
    const parsedUrl = url.parse(req.url);
   
    // If requested url is "/" like "http://localhost:1800/"
    if(parsedUrl.pathname==="/"){
        var filesLink="<ul>";
        res.setHeader('Content-type', 'text/html');
        var filesList=fs.readdirSync("./");
        filesList.forEach(element => {
            if(fs.statSync("./"+element).isFile()){
                filesLink+=`<br/><li><a href='./${element}'>
                    ${element}
                </a></li>` ;        
            }
        });
          
        filesLink+="</ul>";
       
        res.end("<h1>List of files:</h1> " + filesLink);
    }
   
    /* Processing the requested file pathname to
    avoid directory traversal like,
    http://localhost:1800/../fileOutofContext.txt
    by limiting to the current directory only. */
    const sanitizePath = 
    path.normalize(parsedUrl.pathname).replace(/^(\.\.[\/\\])+/, '');
      
    let pathname = path.join(__dirname, sanitizePath);
      
    if(!fs.existsSync(pathname)) {
          
        // If the file is not found, return 404
        res.statusCode = 404;
        res.end(`File ${pathname} not found!`);
    }
    else {
          
        // Read file from file system limit to 
        // the current directory only.
        fs.readFile(pathname, function(err, data) {
            if(err){
                res.statusCode = 500;
                res.end(`Error in getting the file.`);
            } 
            else {
                  
                // Based on the URL path, extract the
                // file extension. Ex .js, .doc, ...
                const ext = path.parse(pathname).ext;
                  
                // If the file is found, set Content-type
                // and send data
                res.setHeader('Content-type',
                        mimeType[ext] || 'text/plain' );
                  
                res.end(data);
            }
        });
    }
}).listen(PORT);
  
console.log('Node.js web server at port 8081 is running..')
