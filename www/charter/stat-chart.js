var colors = [
    "#4682B4", // steelblue
    "#FF6347", // tomato
    "#FFA500", // orange
    "#8FBC8F", // darkseagreen
    "#6A5ACD", // slateblue
    "#BC8F8F", // rosybrown
    "#FF69B4", // hotpink
    "#DDA0DD", // plum
    "#90EE90", //lightgreen
];

var grayColor = "#CCC";
var grayFontColor = "#888";

/*
    Shade color by percentage.
    Source: http://stackoverflow.com/questions/5560248 (shadeColor2 by Pimp Trizkit)
*/
function shadeColor(color, percent) {
    var f=parseInt(color.slice(1),16),t=percent<0?0:255,p=percent<0?percent*-1:percent,R=f>>16,G=f>>8&0x00FF,B=f&0x0000FF;
    return "#"+(0x1000000+(Math.round((t-R)*p)+R)*0x10000+(Math.round((t-G)*p)+G)*0x100+(Math.round((t-B)*p)+B)).toString(16).slice(1);
}

var memUnits = ["bytes", "KiB", "MiB", "GiB"];

var app = {
    // Data
    raw: null,
    meta: null,
    data: [],
    groups: [],
    root: null,

    // Options
    options: {
       drawGroups: true,
       drawOffsets: true,
       drawLegend: true
    },

    // Dimensions
    svgWidth: 900,
    svgHeight: 450,

    chartWidth: 0,
    chartHeight: 0,

    // Scales
    tUnit: "s",
    tScale: null,

    memUnit: "",
    memScale: null,

    // Axes
    x: null,
    y: null,

    // Elements
    marker: null,

    // Utility
    timeToPx: function(ms) {
        return app.x(app.tScale(ms));
    },

    memToPx: function(bytes) {
        return app.y(app.memScale(bytes));
    }
}

//Convert stats to data
var convert = function(x, memOff, level) {
    var ds = {
        title:     x.title,
        tStart:    x.timeStart - app.raw.timeStart,
        tEnd:      x.timeEnd - app.raw.timeStart,
        tDuration: (x.timeEnd - x.timeStart),
        memOff:    memOff + x.memOff,
        memPeak:   memOff + x.memOff + x.memPeak,
        memFinal:  memOff + x.memOff + x.memFinal,
        stats:     x.stats,
        sub:       []
    };

    if(x != app.raw && x.sub.length > 0) {
        ds.level = level;
        app.groups.push(ds);
    } else if(x.sub.length == 0) {
        ds.color = colors[app.data.length % colors.length];
        app.data.push(ds);
    }

    for(var i = 0; i < x.sub.length; i++) {
        ds.sub.push(convert(x.sub[i], memOff + x.memOff, level + 1));
    }

    return ds;
};

var printMeta = function(meta) {
    var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];

    var date = new Date(meta.startTime * 1000);
    var year = date.getFullYear();
    var month = months[date.getMonth()];
    var day = date.getDate();
    var hours = date.getHours();
    var minutes = "0" + date.getMinutes();
    var seconds = "0" + date.getSeconds();

    if(!meta.title) meta.title = "Untitled";

    d3.select("#data h1.title").text(meta.title);
    d3.select("#meta td.timestamp").text(month + " " + day + ", " + year + " - " + hours + ":" + minutes.substr(-2) + ":" + seconds.substr(-2));
    d3.select("#meta td.config").text(meta.config);
    d3.select("#meta td.input").text(meta.input);
    d3.select("#meta td.input-size").text(formatMem(meta.inputSize));
    d3.select("#meta td.output").text(meta.output);
    d3.select("#meta td.output-size").text(formatMem(meta.outputSize));
    d3.select("#meta td.rate").text(formatPercent(meta.rate));
}

var drawChart = function(raw) {
    app.data = [];
    app.groups = [];

    app.root = convert(raw, raw.memOff, -1);

    // Define dimensions
    var groupLevelMax = d3.max(app.groups, function(g) { return g.level; });
    var groupLevelIndent = function(level) {
        return (groupLevelMax - level) * 30;
    };

    var margin = {
        top: 10,
        left: 55,
        bottom: 50,
        right: 10
    };

    if(app.options.drawGroups && app.groups.length > 0) {
        margin.top += groupLevelIndent(-1);
    }

    if(app.options.drawLegend) {
        var longestTitle = d3.max(app.data, function(d) { return d.title.length; });
        margin.right += longestTitle * 8;
    }

    app.chartWidth = app.svgWidth - margin.left - margin.right;
    app.chartHeight = app.svgHeight - margin.top - margin.bottom;

    // Define scales
    var tDuration = raw.timeEnd - raw.timeStart;

    app.tScale = d3.scale.linear()
                .range([0, tDuration / 1000])
                .domain([0, tDuration]);

    var memDiv = 1;
    {
        var u = 0;
        var memPeak = raw.memPeak;
        while(u < memUnits.length && memPeak > 1024) {
            memPeak /= 1024.0;
            memDiv *= 1024;
            u++;
        }

        app.memUnit = memUnits[u];
    }

    app.memScale = d3.scale.linear()
                    .range([0, raw.memPeak / memDiv])
                    .domain([0, raw.memPeak]);

    // Define axes
    app.x = d3.scale.linear()
                .range([0, app.chartWidth])
                .domain(app.tScale.range());

    app.y = d3.scale.linear()
                .range([app.chartHeight, 0])
                .domain(app.memScale.range());

    var xAxis = d3.svg.axis().scale(app.x).orient("bottom").ticks(15);
    var yAxis = d3.svg.axis().scale(app.y).orient("left");

    // Clear chart
    var svg = d3.select("#chart svg");
    svg.html("");

    // Create new chart
    var chart = svg
        .attr("width", app.svgWidth)
        .attr("height", app.svgHeight)
        .append("g")
        .attr("class", "zoom")
        .attr("transform", "scale(1.0)")
        .append("g")
        .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

    chart.on("mousemove", chartMouseMove);
    chart.on("mouseout", chartMouseOut);

    // Draw background - TODO: optional?
    /*chart.append("rect")
        .attr("class", "back")
        .attr("x", "0")
        .attr("y", "0")
        .attr("width", app.chartWidth)
        .attr("height", app.chartHeight)
        .attr("fill", "white");*/

    // Draw bars
    var bar = chart.selectAll("g.bar")
        .data(app.data).enter()
        .append("g")
        .attr("class", "bar")
        .attr("transform", function(d) {
            return "translate(" + app.timeToPx(d.tStart) + ",0)";
         });

    // Main bar
    bar.append("rect")
        .attr("class", "mem")
        .attr("x", "0")
        .attr("y", function(d) { return app.memToPx(d.memPeak); })
        .attr("width", function(d) { return app.timeToPx(d.tDuration); })
        .attr("height", function(d) { return app.chartHeight - app.memToPx(d.memPeak); })
        .attr("fill", function(d) { return d.color; });

    // Offset bar
    if(app.options.drawOffsets) {
        bar.append("rect")
            .attr("class", "mem")
            .attr("x", "0")
            .attr("y", function(d) { return app.memToPx(d.memOff); })
            .attr("width", function(d) { return app.timeToPx(d.tDuration); })
            .attr("height", function(d) { return app.chartHeight - app.memToPx(d.memOff); })
            .attr("fill", function(d) { return shadeColor(d.color, -0.25); });
    }

    // Draw legend
    if(app.options.drawLegend) {
        var legend = chart.selectAll("g.legend")
                    .data(app.data).enter()
                    .append("g")
                    .attr("class", "legend")
                    .attr("transform", function(d) {
                        return "translate(" + (app.x.range()[1] + 1) + ",0)";
                    });

        legend.append("rect")
            .attr("fill", function(d) { return (app.timeToPx(d.tDuration) < 1.0) ? grayColor : d.color; })
            .attr("x", "0")
            .attr("y", function(d, i) { return (i * 1.5) + "em" })
            .attr("width", "1em")
            .attr("height", "1em");

        legend.append("text")
            .attr("x", "1.25em")
            .attr("y", function(d, i) { return (i * 1.5) + "em" })
            .attr("dy", "1em")
            .text(function(d) { return d.title; })
            .style("fill", function(d) { return (app.timeToPx(d.tDuration) < 1.0) ? grayFontColor : "inherit"; })
            .style("font-style", function(d) { return (app.timeToPx(d.tDuration) < 1.0) ? "italic" : "normal"; })
            .style("text-decoration", function(d) { return (app.timeToPx(d.tDuration) < 1.0) ? "line-through" : "none"; });
    }

    // Draw groups
    if(app.options.drawGroups) {
        var group = chart.selectAll("g.group")
            .data(app.groups).enter()
            .append("g")
            .attr("class", "group")
            .attr("transform", function(d) {
                return "translate(" + app.timeToPx(d.tStart) + ",0)";
            });

        group.append("text")
            .attr("x", function(d) { return app.timeToPx(d.tDuration / 2); })
            .attr("y", function(d) { return app.memToPx(d.memPeak) - groupLevelIndent(d.level); })
            .attr("dy", "-12")
            .style("font-weight", "bold")
            .style("text-anchor", "middle")
            .text(function(d) { return d.title; });

        group.append("line")
            .attr("x1", "0")
            .attr("x2", "0")
            .attr("y1", function(d) { return app.memToPx(d.memPeak) - groupLevelIndent(d.level); })
            .attr("y2", app.chartHeight);

        group.append("line")
            .attr("x1", function(d) { return app.timeToPx(d.tDuration); })
            .attr("x2", function(d) { return app.timeToPx(d.tDuration); })
            .attr("y1", function(d) { return app.memToPx(d.memPeak) - groupLevelIndent(d.level); })
            .attr("y2", app.chartHeight);

        group.append("line")
            .attr("x1", "0")
            .attr("x2", function(d) { return app.timeToPx(d.tDuration); })
            .attr("y1", function(d) { return app.memToPx(d.memPeak) - groupLevelIndent(d.level); })
            .attr("y2", function(d) { return app.memToPx(d.memPeak) - groupLevelIndent(d.level); });

        group.append("line")
            .attr("x1", function(d) { return app.timeToPx(d.tDuration / 2); })
            .attr("x2", function(d) { return app.timeToPx(d.tDuration / 2); })
            .attr("y1", function(d) { return app.memToPx(d.memPeak) - groupLevelIndent(d.level) - 10; })
            .attr("y2", function(d) { return app.memToPx(d.memPeak) - groupLevelIndent(d.level); });

        group.selectAll("line")
            .style("stroke", "rgba(0, 0, 0, 0.75)")
            .style("stroke-width", "1")
            .style("stroke-dasharray", "5,2");
    }

    // Draw axes
    chart.append("g")
        .attr("class", "axis")
        .attr("transform", "translate(0," + app.chartHeight + ")")
        .call(xAxis);

    chart.append("text")
        .attr("class", "axis-label")
        .attr("transform", "translate(" + app.chartWidth / 2 + "," + app.chartHeight + ")")
        .attr("dy", "3em")
        .style("font-weight", "bold")
        .style("font-style", "italic")
        .style("text-anchor", "middle")
        .text("Time / " + app.tUnit);

    chart.append("g")
        .attr("class", "axis")
        .call(yAxis);

    chart.append("text")
        .attr("class", "axis-label")
        .attr("transform", "translate(0," + app.chartHeight / 2 + ") rotate(-90)")
        .attr("dy", "-3em")
        .style("font-weight", "bold")
        .style("font-style", "italic")
        .style("text-anchor", "middle")
        .text("Memory Peak / " + app.memUnit);

    chart.selectAll(".axis path.domain")
        .style("fill", "none")
        .style("stroke", "black")
        .style("shape-rendering", "crispEdges");

    chart.selectAll(".axis g.tick line")
        .style("stroke", "black");

    // Marker
    app.marker = chart.append("g")
        .attr("class", "marker")
        .attr("transform", "translate(0, 0)");

    app.marker.append("line")
        .attr("x1", "0")
        .attr("x2", "0")
        .attr("y1", 0)
        .attr("y2", app.chartHeight)
        .style("stroke", "black")
        .style("stroke-width", "1");

    app.marker.append("circle")
        .attr("cx", "0")
        .attr("cy", "0")
        .attr("r", "3")
        .style("fill", "black");

    // Data table
    printDataTable();

    // Post
    d3.select("#dropzone-wrapper").style("display", "none");
    d3.select("#footer").style("display", "none");
    d3.select("#chart").style("display", "block");
    d3.select("#options").style("display", "block");

    updateZoomText(1.0);
    hideMarker();
};

var printPhase = function(parentElem, d) {
    var phaseNode = parentElem.insert("div").attr("class", "phase");

    var shortPhase = (app.timeToPx(d.tDuration) < 1.0);
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

    printPhase(data, app.root);
};

var fillStatTable = function(d, e) {
    var dur = d.tEnd - d.tStart;
    var durPct = dur / app.tScale.invert(app.tScale.range()[1]);

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
    return app.tScale(ms).toFixed(3) + " " + app.tUnit;
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
    var t = app.tScale.invert(app.x.invert(xpos));

    for(var i = 0; i < app.data.length; i++) {
        var d = app.data[i];
        if(t >= d.tStart && t <= d.tEnd) {
            var memY = app.memToPx(d.memPeak);

            app.marker.attr("transform", "translate(" + xpos + ",0)");
            app.marker.select("circle").attr("cy", memY);

            return {
                data: d,
                y:    memY
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
    if(mx >= 0 && mx <= app.chartWidth && my >= 0 && my <= app.chartHeight) {
        var marker = setMarker(mx);
        if(marker && marker.data) {
            showMarker();

            var d = marker.data;
            if(d.tStart != cachedStart) {
                cachedStart = d.tStart;
                cachedTop = marker.y;

                var dur = d.tEnd - d.tStart;
                var durPct = dur / app.tScale.invert(app.tScale.range()[1]);

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
    if(mx >= 0 && mx <= app.chartWidth && my >= 0 && my <= app.chartHeight) {
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
        .attr("width", app.svgWidth * zoom)
        .attr("height", app.svgHeight * zoom);
    d3.select("#chart g.zoom").attr("transform", "scale(" + zoom + ")");

    updateZoomText(zoom);
}

var redrawChart = function() {
    drawChart(app.raw);
    if(app.meta) printMeta(app.meta);
}

var loadJSON = function(json) {
    d3.select("#json")[0][0].value = json;
    d3.select("#json-error").style("display", "none");
    try {
        var x = JSON.parse(json);
        if(x.meta) {
            app.raw = x.data;
            app.meta = x.meta;
        } else {
            app.raw = x;
        }

        redrawChart();
    } catch(err) {
        d3.select("#json-error-message").text(err.message);
        d3.select("#json-error").style("display", "block");
    }
}

