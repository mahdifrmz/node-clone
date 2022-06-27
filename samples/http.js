var socket = require('socket')

const response = ''
    + 'HTTP/1.1 200 OK\n'
    + 'Content-Length: 12\n'
    + 'Content-Type: text/html\n'
    + 'Connection: Closed\n\n'
    + 'hello world\n'

socket.createServer(8080, function (server) {
    server.listen(function (client) {
        client.recieve(function (message) {
            print('client requested')
            client.send(response)
        })
    })
})