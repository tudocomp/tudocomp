function indexArray(n, base = 0) {
    result = [];
    for(var i = 0; i < n; i++) result.push(i + base);
    return result;
}

function suffixArray(string, base = 0) {
    n = string.length;
    suffixes = [];
    for(var i = 0; i < n; i++)
        suffixes[i] = string.substr(i, n - i);
    suffixes.sort();
    return suffixes.map(function(suffix) {
        return n - suffix.length + base;
    });
}

function inverseSuffixArray(suffixArray, base = 0) {
    result = [];
    for(var i = 0; i < suffixArray.length; i++)
        result[suffixArray[i] - base] = i + base;
    return result;
}

function phiArray(suffixArray, inverseSuffixArray, base = 0) {
    result = [];
    for(var i = 0; i < suffixArray.length; i++)
        if(inverseSuffixArray[i] != base)
            result[i] = suffixArray[inverseSuffixArray[i] - base - 1];
        else
            result[i] = "-";
    return result;
}

function lcp(string, pos1, pos2) {
    result = 0;
    while(string[pos1 + result] == string[pos2 + result])
        result++;
    return result;
}

function lcpArray(string, suffixArray, base = 0) {
    string += '\0';
    var result = [0];
    for(var i = 1; i < suffixArray.length; i++)
        result.push(lcp(string, suffixArray[i] - base, suffixArray[i - 1] - base));
    return result;
}

function plcpArray(inverseSuffixArray, lcpArray, base = 0) {
    var result = [];
    for(var i = 0; i < inverseSuffixArray.length; i++)
        result.push(lcpArray[inverseSuffixArray[i] - base]);
    return result;
}

function psiArray(suffixArray, inverseSuffixArray, base = 0) {
    var result = [];
    for(var i = 0; i < suffixArray.length; i++)
        if(suffixArray[i] - base + 1 < suffixArray.length)
            result.push(inverseSuffixArray[suffixArray[i] - base + 1]);
        else 
            result.push("-");
    return result;
}

function firstRow(string, suffixArray, base = 0) {
    var n = string.length;
    var result = "";
    for(var i = 0; i < n; i++)
        result += string[suffixArray[i] - base];
    return result;
}

function bwt(string, suffixArray, base = 0) {
    var n = string.length;
    var result = "";
    for(var i = 0; i < n; i++)
        result += string[(suffixArray[i] - base + n - 1) % n];
    return result;
}

function lfArray(suffixArray, inverseSuffixArray, base = 0) {
    var n = suffixArray.length;
    var result = [];
    for(var i = 0; i < n; i++) {
        var sa = (suffixArray[i] - base + n - 1) % n;
        result[i] = inverseSuffixArray[sa];
    }
    return result;
}

function padRight(str, char, len) {
    while(str.length < len) str += char;
    return str;
}

function padLeft(str, char, len) {
    while(str.length < len) str = char + str;
    return str;
}

function arrayToString(array, sep = " ", base = 0, hex = false) {
    hex = hex ? 16 : 10;
    var width = ("" + (array.length + base - 1).toString(hex)).length
    var result = "";
    for(var i = 0; i < array.length - 1; i++) 
        result += padLeft("" + array[i].toString(hex).toUpperCase(), ' ', width) + sep;
    result += padLeft("" + array[array.length - 1].toString(hex).toUpperCase(), ' ', width);
    return result;
}

function stringToString(string, sep = " ", base = 0, hex = false) {
    hex = hex ? 16 : 10;
    string = string.replace("\0", "$");
    var width = ("" + (string.length + base - 1).toString(hex)).length
    var result = "";
    for(var i = 0; i < string.length - 1; i++) 
        result += padLeft("" + string[i], ' ', width) + sep;
    result += padLeft("" + string[string.length - 1], ' ', width);
    return result;
}
