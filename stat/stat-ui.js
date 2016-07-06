var urlPattern = new RegExp('^(https?:\\/\\/)?'+ // protocol
    '((([a-z\\d]([a-z\\d-]*[a-z\\d])*)\\.?)+[a-z]{2,}|'+ // domain name
    '((\\d{1,3}\\.){3}\\d{1,3}))'+ // OR ip (v4) address
    '(\\:\\d+)?(\\/[-a-z\\d%_.~+]*)*'+ // port and path
    '(\\?[;&a-z\\d%_.~+=-]*)?'+ // query string
    '(\\#[-a-z\\d_]*)?$','i'); // fragment locator

var loadJSONfromURL = function(url) {
    if(urlPattern.test(url)) {
        //get using asynchronous http request

        var ajax = new XMLHttpRequest();
        ajax.onreadystatechange = function() {
            if(ajax.readyState == 4 && ajax.status == 200) {
                loadJSON(ajax.responseText);
            }
        };

        ajax.open("GET", url, true);
        ajax.send(null);
    }
}

var updateZoomText = function(zoom) {
    d3.select("#options .zoom-label").text(
        zoom.toFixed(1)
        + " (" + (app.svgWidth * zoom).toFixed(0)
        + " x " + (app.svgHeight * zoom).toFixed(0)
        + ")"
    );
}

d3.select("#options button.close").on("click", function() {
    d3.select("#json")[0][0].value = "";
    d3.select("#dropzone").style("display", "block");
    d3.select("#sample").style("display", "block");
    d3.select("#chart").style("display", "none");
    d3.select("#options").style("display", "none");
});

d3.select("#options button.svg").on("click", function() {
    window.open("data:image/svg+xml;base64," +
        btoa(d3.select("#svg-container").html()));
});

d3.select("#options .zoom")
    .on("input", function() {
        setZoom(parseFloat(this.value));
    })
    .on("dblclick", function() {
        this.value = 1.0;
        setZoom(1.0);
    });

d3.select("#options .groups").on("change", function() {
    app.options.drawGroups = this.checked;
    redrawChart();
});

d3.select("#options .offsets").on("change", function() {
    app.options.drawOffsets = this.checked;
    redrawChart();
});

d3.select("#options .legend").on("change", function() {
    app.options.drawLegend = this.checked;
    redrawChart();
});


d3.select("#dropzone")
    .on("dragover", function() {
        var e = d3.event;

        e.stopPropagation();
        e.preventDefault();
        e.dataTransfer.dropEffect = 'copy';
    })
    .on("drop", function() {
        var e = d3.event;

        e.stopPropagation();
        e.preventDefault();

        if(e.dataTransfer.files.length > 0) {
            var file = e.dataTransfer.files[0];
            var reader = new FileReader();
            reader.onload = function(e) {
                var x = e.target.result.indexOf("base64,");
                if(x >= 0) {
                    loadJSON(atob(e.target.result.substr(x + 7)));
                }
            };

            reader.readAsDataURL(file);
        } else if(e.dataTransfer.items.length > 0) {
            var item = e.dataTransfer.items[0];
            if(item.kind == "string") {
                item.getAsString(function(url) {
                    loadJSONfromURL(url);
                });
            }
        }
    });

d3.select("#json-load").on("click", function() {
    loadJSON(d3.select("#json")[0][0].value);
});

