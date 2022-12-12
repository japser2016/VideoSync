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
    '.mp4': 'video/mp4',
    '.webm': 'video/mp4'
};

// Create a server object
var PORT = 8081

var playing = 0;
var timestamp = 0.0;
var changed = 0;
var index = 0;

fs.writeFile("../webserver-output", "0\n0\n0\n", function(err) {
    if(err) throw err;
});


http.createServer( (req, res) => {
    
    if (req.method == "GET") {
        
        // Parsing the requested URL
        const parsedUrl = url.parse(req.url);
        if(parsedUrl.pathname==="/input") {
            fs.readFile("../syncserver-output", function(err, data) {
                if (!err) {
                    var sync_playing = Number(data.slice(0,1));
                    //console.log("p:"+sync_playing);
                    var temp1 = data.indexOf("\n");
                    //console.log("temp1:"+temp1);
                    var temp2 = data.indexOf("\n", temp1 + 1);
                    //console.log("temp2:"+temp2);
                    var sync_timestamp = parseFloat(data.slice(temp1 + 1,temp2));
                    //console.log("ts:"+sync_timestamp);
                    var sync_index = Number(data.slice(temp2 + 1));
                    //console.log("ind:"+sync_index);
                    
                    if (sync_index > index) {
                        index = sync_index;
                        playing = sync_playing;
                        timestamp = sync_timestamp;
                        changed = 1;
                    }
                }
            });
            
            const data = {
                "changed": changed,
                "playing": playing,
                "timestamp": timestamp
            };
            if(changed) {
                console.log("webserver updating video metadata with\nplaying: "+playing+"\ntimestamp: "+timestamp);
                changed = 0;
            }
            //res.setHeader('Content-type', 'application/json');
            res.end(JSON.stringify(data));
        }
        else {
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
        }
    } else if(req.method == "POST") {
        req.on('data', function(data) {
            var req_data = JSON.parse(data);
            //console.log("playing: " + req_data.playing);
            //console.log("timestamp: " + req_data.timestamp);
            playing = req_data.playing;
            timestamp = req_data.timestamp;
            index = index + 1;
            
            //SEND TO SYNC SERVER
            var output = playing + "\n" + timestamp + "\n" + index + "\n";
            fs.writeFile('../webserver-output', output, function (err) {
                if (err) throw err;
            });
            
        });
        
    }
}).listen(PORT);

console.log('Node.js web server at port '+PORT+' is running..')

