function OptionList(onChange) {
    this.dictionary = {};
    this.onChange = onChange;
}

OptionList.prototype.add = function(domCheckbox) {
    this.dictionary[domCheckbox.dataset.opt] = domCheckbox;
    domCheckbox.onchange = this.onChange;
};

OptionList.prototype.getEnabled = function() {
    var result = "";
    for(var key in this.dictionary)
        if(this.enabled(key))
            result += key + '-';
    return result.substr(0, Math.max(0, result.length - 1));
};

OptionList.prototype.setEnabled = function(optList) {
    for(var key in this.dictionary)
        this.disable(key);
    var opts = optList.split('-');
    for(var i = 0; i < opts.length; i++)
        this.enable(opts[i]);
};

OptionList.prototype.enabled = function(optName) {
    return this.dictionary[optName] 
        && this.dictionary[optName].checked;
};

OptionList.prototype.enable = function(optName) {
    var el = this.dictionary[optName];
    if(el) {
        el.checked = true;
        this.onChange();
    }
};

OptionList.prototype.disable = function(optName) {
    var el = this.dictionary[optName];
    if(el) {
        el.checked = false;
        this.onChange();
    }
};

OptionList.prototype.toggle = function(optName) {
    if(this.enabled(optName)) this.disable(optName);
    else this.enable(optName);
};
