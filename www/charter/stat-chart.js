// Application data
var app = {
    raw: null,
    options: {
        svgWidth: 900,
        svgHeight: 450,
        drawGroups: true,
        drawOffsets: true,
        drawLegend: true,
    },

    chart: null,
    marker: null,
}

// print meta information
var printMeta = function() {
    var meta = app.chart.data.meta;
    if(meta) {
        var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];

        var date = new Date(meta.startTime * 1000);
        var year = date.getFullYear();
        var month = months[date.getMonth()];
        var day = date.getDate();
        var hours = date.getHours();
        var minutes = "0" + date.getMinutes();
        var seconds = "0" + date.getSeconds();

        if(!meta.title) meta.title = "Untitled";

        d3.select("#meta").style("display", "block");
        d3.select("#meta h1").text(meta.title);
        d3.select("#meta td.timestamp").text(month + " " + day + ", " + year + " - " + hours + ":" + minutes.substr(-2) + ":" + seconds.substr(-2));
        d3.select("#meta td.config").text(meta.config);
        d3.select("#meta td.input").text(meta.input);
        d3.select("#meta td.input-size").text(formatMem(meta.inputSize));
        d3.select("#meta td.output").text(meta.output);
        d3.select("#meta td.output-size").text(formatMem(meta.outputSize));
        d3.select("#meta td.rate").text(formatPercent(meta.rate));
    } else {
        d3.select("#meta").style("display", "none");
    }
}

// redraw chart
var drawChart = function(data) {
    // create chart object
    app.chart = new chart(app.raw, app.options);

    // insert SVG
    var container = d3.select("#svg-container");
    container.html(app.chart.svg);

    // print or hide meta information
    printMeta();

    // print data table
    printDataTable();

    // create marker
    var gChart = d3.select("#chart svg > g > g")
    app.marker = gChart.append("g")
        .attr("class", "marker")
        .attr("transform", "translate(0, 0)");

    app.marker.append("line")
        .attr("x1", "0")
        .attr("x2", "0")
        .attr("y1", 0)
        .attr("y2", app.chart.height)
        .style("stroke", "black")
        .style("stroke-width", "1");

    app.marker.append("circle")
        .attr("cx", "0")
        .attr("cy", "0")
        .attr("r", "3")
        .style("fill", "black");

    // initially hide marker
    hideMarker();

    // hook mouse events
    gChart.on("mousemove", chartMouseMove);
    gChart.on("mouseout", chartMouseOut);

    // post actions
    d3.select("#dropzone-wrapper").style("display", "none");
    d3.select("#footer").style("display", "none");
    d3.select("#chart").style("display", "block");
    d3.select("#options").style("display", "block");

    updateZoomText(1.0);
};

var printPhase = function(parentElem, d) {
    var phaseNode = parentElem.insert("div").attr("class", "phase");

    var shortPhase = (app.chart.timeToPx(d.tDuration) < 1.0);
    var titleNode = phaseNode.insert("div")
        .attr("class", "title " + ((d.sub.length > 0) ? "group" : "leaf"))
        .style("border-left-color", shortPhase ? grayColor : d.color)
        .text(d.title);

    var e = phaseNode.insert("div")
        .attr("class", "content")
        .html(d3.select("#data-template").html());

    fillStatTable(d, e);

    if(d.sub.length > 0) {
        e.insert("div").attr("class", "sub").text("Sub phases:");
        for(var i = 0; i < d.sub.length; i++) {
            printPhase(e, d.sub[i]);
        }
    }
};

var printDataTable = function() {
    var data = d3.select("#data-content");
    data.html("");

    printPhase(data, app.chart.data.root);
};

var fillStatTable = function(d, e) {
    var dur = d.tEnd - d.tStart;
    var durPct = dur / app.chart.data.root.tDuration;

    e.select(".title").text(d.title);
    e.select(".start").text(formatTime(d.tStart));
    e.select(".duration").text(
        formatTime(dur) + " (" + formatPercent(durPct) + ")");
    e.select(".mempeak").text(formatMem(d.memPeak));
    e.select(".memlocal").text(formatMem(d.memPeak - d.memOff));
    e.select(".memoffset").text(formatMem(d.memOff));
    e.select(".memfinal").text(formatMem(d.memFinal));
    e.select(".memadd").text(formatMem(d.memFinal - d.memOff));

    var ext = e.select("tbody.ext").html("");
    if(d.stats.length > 0) {
        var tr = ext.selectAll("tr").data(d.stats).enter().append("tr");

        tr.append("th").text(function(kv) { return kv.key + ":"; });
        tr.append("td").text(function(kv) { return kv.value; });
    }
}

// Formatting functions
var formatTime = function(ms) {
    if(ms < 1000) {
        return ms + " ms";
    } else {
        return (ms / 1000).toFixed(3) + " s";
    }
};

var formatMem = function(mem) {
    var u = 0;
    while(u < memUnits.length && Math.abs(mem) > 1024) {
        mem /= 1024.0;
        u++;
    }

    return (u > 0 ? mem.toFixed(2) : mem.toString()) + " " + memUnits[u];
}

var formatPercent = function(pct) {
    return (pct * 100.0).toFixed(2) + " %";
}

// Utilities
var showMarker = function() {
    app.marker.style("display", null);
}

var hideMarker = function() {
    app.marker.style("display", "none");
}

var setMarker = function(xpos) {
    var t = app.chart.pxToTime(xpos);

    for(var i = 0; i < app.chart.data.data.length; i++) {
        var d = app.chart.data.data[i];
        if(t >= d.tStart && t <= d.tEnd) {
            var memY = app.chart.memToPx(d.memPeak);

            app.marker.attr("transform", "translate(" + xpos + ",0)");
            app.marker.select("circle").attr("cy", app.chart.height - memY);

            return {
                data: d,
                y:    app.chart.height - memY
            };
        }
    }

    return null;
};

// Events
var cachedStart = -1;

var chartMouseMove = function() {
    var m = d3.mouse(this);

    var mx = m[0], my = m[1];
    if(mx >= 0 && mx <= app.chart.width && my >= 0 && my <= app.chart.height) {
        var marker = setMarker(mx);
        if(marker && marker.data) {
            showMarker();

            var d = marker.data;
            if(d.tStart != cachedStart) {
                cachedStart = d.tStart;
                cachedTop = marker.y;

                var dur = d.tEnd - d.tStart;
                var durPct = dur / app.chart.data.root.tDuration;

                var tip = d3.select("#tip");
                fillStatTable(d, tip);
            }

            var mdoc = d3.mouse(document.documentElement);
            d3.select("#tip")
                .style("display", "inline")
                .style("left", mdoc[0] + "px")
                .style("top", mdoc[1] + "px");
        } else {
            hideMarker();
            d3.select("#tip").style("display", "none");
        }
    }
};

var chartMouseOut = function() {
    var m = d3.mouse(this);

    var mx = m[0], my = m[1];
    if(mx >= 0 && mx <= app.chart.width && my >= 0 && my <= app.chart.height) {
        var marker = setMarker(mx);
        if(marker && marker.data && my >= marker.y) {
            return;
        }
    }

    hideMarker();
    d3.select("#tip").style("display", "none");
};

var setZoom = function(zoom) {
    d3.select("#chart svg")
        .attr("width", app.options.svgWidth * zoom)
        .attr("height", app.options.svgHeight * zoom);
    d3.select("#chart g.zoom").attr("transform", "scale(" + zoom + ")");

    updateZoomText(zoom);
}

var redrawChart = function() {
    drawChart();
}

var loadJSON = function(json) {
    d3.select("#json")[0][0].value = json;
    d3.select("#json-error").style("display", "none");
    try {
        app.raw = JSON.parse(json);
        redrawChart();
    } catch(err) {
        console.log(err);
        d3.select("#json-error-message").text(err.message);
        d3.select("#json-error").style("display", "block");
    }
}

