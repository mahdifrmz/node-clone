const src = ''
const dest = ''

var fs = require('fs')

setTimeout(function () {
    fs.readFile(src, function (data) {
        fs.writeFile(dest, data, function () {
            print("copying done!");
        })
    })
}, 2000)