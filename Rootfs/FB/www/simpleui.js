/*
This file is a part of FBrowser project.
https://github.com/44670/FBrowser
License: LGPL v3
*/

function $id(id) {
    return document.getElementById(id);
}

function $html(html) {
    var div = document.createElement('div');
    div.innerHTML = html;
    return div.firstElementChild;
}

var body = document.body;

// === Page Manager ===
var showingPage;
var pageStack = []; 
var pageObjects = {};
var navElement = $id('nav');
var navTitle = navElement.getElementsByClassName('title')[0];
var navBack = navElement.getElementsByClassName('nav-back')[0];

function pageBeforeCreated() {
    var el = showingPage.el;
    var inputs = el.getElementsByTagName('input');
    var divs = el.getElementsByTagName('div');
    for (var i = 0; i < inputs.length; i++) {
        var inp = inputs[i];
        if (inp.classList.contains('txt-swkbd')) {
            swkbdBindElement(inp);
        }
    }
    listSetupElements(divs);
}

function onPageStackUpdated() {
    if (showingPage) {
        showingPage.shown = false;
        showingPage.el.hidden = true;
        try {
            if (showingPage.callback) {
                showingPage.callback('hide')
            }
        } catch (e) {
            console.log(e);
        }
    }
    showingPage = pageStack[pageStack.length - 1];
    if (!showingPage) {
        console.error('No page to show');
    }
    showingPage.shown = true;
    showingPage.el.hidden = false;
    var title = showingPage.el.dataset.title || ''
    navTitle.innerText = title;
    navBack.hidden = pageStack.length <= 1;
    if (!showingPage.created) {
        showingPage.created = true;
        pageBeforeCreated(showingPage);
        try {
            if (showingPage.callback) {
                showingPage.callback('create')
            }
        } catch (e) {
            console.log(e);
        }
    }
    navElement.hidden = showingPage.nonav;
    if (showingPage.callback) {
        showingPage.callback('show');
    }
}

function pageGetObjectByElement(el) {
    var id = el.id
    if (!id) {
        console.error('Page element must have an id');
        return;
    }
    if (!pageObjects[id]) {
        pageObjects[id] = {
            el: el,
            created: false,
            shown: false,
            nonav: el.classList.contains('page-nonav'),
        }
    }
    return pageObjects[id];
}

function pageSetCallback(id, callback) {
    var el = $id(id);
    if (!el) {
        console.error('Page not found: ' + id);
        return;
    }
    pageGetObjectByElement(el).callback = callback;
}

function pageGoto(id) {
    var el = $id(id)
    if (!el) {
        console.error('Page not found: ' + id);
        return;
    }
    pageStack.push(pageGetObjectByElement(el));
    onPageStackUpdated();
}

function pageGoBack() {
    if (pageStack.length <= 1) {
        console.error('Cannot go back');
        return;
    }
    pageStack.pop();
    onPageStackUpdated();
}

// === Modal dialogs ===
var modalElement = $html('<div class="modal" tabindex="-1" hidden></div>');
var modalState = {
    callback: null
}
body.appendChild(modalElement);

function modalDismiss(value) {
    modalElement.innerHTML = '';
    modalElement.hidden = true;
    if (modalState.callback) {
        modalState.callback(value);
        modalState.callback = null;
    }
}

function modalShow(el, callback) {
    modalDismiss(undefined);
    modalElement.appendChild(el);
    modalState.callback = callback;
    modalElement.hidden = false;
}

function modalMakeDialog(content, buttons) {
    var dialog = $html('<div class="dialog"></div>');
    var body = $html('<div class="dialog-body"></div>');

    var footer = $html('<div class="dialog-footer"></div>');
    for (var i = 0; i < buttons.length; i++) {
        var button = $html('<button class="btn"></button>');
        button.innerText = buttons[i];
        button.data = i;
        button.onclick = function (e) {
            modalDismiss(this.data);
        };
        footer.appendChild(button);
    }
    dialog.appendChild(body);
    dialog.appendChild(footer);
    modalElement.appendChild(dialog);
    return dialog;
}

var alertDialog
var confirmDialog

function modalAlert(content, callback) {
    if (!alertDialog) {
        alertDialog = modalMakeDialog('', ['OK']);
    }
    alertDialog.getElementsByClassName('dialog-body')[0].innerText = content;
    modalShow(alertDialog, callback);
}

function modalConfirm(content, callback) {
    if (!confirmDialog) {
        confirmDialog = modalMakeDialog('', ['Cancel', 'OK']);
    }
    confirmDialog.getElementsByClassName('dialog-body')[0].innerText = content;
    modalShow(confirmDialog, callback);
}

// === Software keyboard ===
var swkbdElement;
var keyMaps = [['qwertyuiop', 'asdfghjkl', 'zxcvbnm'], ['QWERTYUIOP', 'ASDFGHJKL', 'ZXCVBNM'],  ['1234567890', '-/:;()$&@', '.,?!%*#'], ['_[]{}^+=\\|', '~<>"\'`', '       ']];
var swkbdState = {
    currentKeyMap: 0,
    keys:[],
    targetElement: null,
}

function swkbdSetMap(km) {
    swkbdState.currentKeyMap = km;
    var keyMap = keyMaps[km];
    var keys = swkbdState.keys;
    for (var row = 0; row < 3; row++) {
        var offset = (row == 2) ? 2 : 0;
        for (var col = 0; col < keyMap[row].length; col++) {
            var key = keys[row] [ col + offset];
            key.innerText = keyMap[row][col];
            key._k = keyMap[row][col];
        }
    }
}

function swkbdOnKeyClick() {
    var k = this._k;
    var targetElement = swkbdState.targetElement;
    if (k.length == 1) {
        // A single character
        if (targetElement) {
            swkbdState.targetElement.value += k;
        }
        return;
    }
    if (k == 'ok') {
        var e = new KeyboardEvent('keydown', {
            'key': 'Enter',
        });
        if (targetElement) {
            if (targetElement.keydown) {
                targetElement.onkeydown(e);
            }
        }
        swkbdHide();
    } else if (k == 'bksp') {
        if (targetElement) {
            targetElement.value = targetElement.value.slice(0, -1);
        }
    } else if (k == 'lang') {
        var nextKeyMap = swkbdState.currentKeyMap + 1;
        if (nextKeyMap >= keyMaps.length) {
            nextKeyMap = 0;
        }
        swkbdSetMap(nextKeyMap);
    }

}

function swkbdInit() {
    if (swkbdElement) {
        return;
    }
    swkbdElement = $html('<div class="swkbd" hidden></div>');
    body.appendChild(swkbdElement);
    var rows = []
    for (var i = 0; i < 3; i++) {
        var row = $html('<div class="swkbd-row"></div>');
        rows.push(row);
        swkbdElement.appendChild(row);
    }
    var keys = swkbdState.keys;
    var keysInRow = [10, 10, 10];
    
    for (var i = 0; i < rows.length; i++) {
        var rowKeys =[];
        var row = rows[i];
        for (var j = 0; j < keysInRow[i]; j++) {
            var key = $html('<div class="swkbd-key"></div>');
            key.onclick = swkbdOnKeyClick;
            row.appendChild(key);
            rowKeys.push(key);
        }
        keys.push(rowKeys);
    }
    keys[1][9].innerText = '<-';
    keys[1][9]._k = 'bksp';
    keys[2][0].innerText = 'Aa.';
    keys[2][0]._k = 'lang';
    keys[2][1].innerText = '[ ]'
    keys[2][1]._k = ' ';
    keys[2][9].innerText = 'OK'
    keys[2][9]._k = 'ok';


    swkbdSetMap(0);
}

function swkbdShow(targetElement) {
    swkbdInit();
    swkbdState.targetElement = targetElement;
    swkbdElement.hidden = false;
}

function swkbdHide() {
    swkbdState.targetElement = null;
    swkbdElement.hidden = true;
}

function swkbdOnFocus() {
    swkbdShow(this);
}

function swkbdOnBlur() {
}

function swkbdBindElement(element) { 
    element.onfocus = swkbdOnFocus;
    element.onblur = swkbdOnBlur;

}

// === List view ===
function listSetupElements(divs) {
    for (var i = 0; i < divs.length; i++) {
        var div = divs[i];
        var classList = div.classList;
        if (!classList.contains('list-item')) {
            continue;
        }
        if (classList.contains('list-item-switch')) {
            var el = $html('<label class="switch"><input type="checkbox"><span class="slider"></span></label>');
            div.appendChild(el);
        }

    }
}



window.$ui = {
    $id: $id,
    $html: $html,
    modal: {
        alert: modalAlert,
        confirm: modalConfirm
    },
    page: {
        go: pageGoto,
        back: pageGoBack,
        on: pageSetCallback
    }
}


document.addEventListener('DOMContentLoaded', function () {
    navBack.onclick = function () {
        pageGoBack();
    }

    var mainPage
    var pages = document.getElementsByClassName('page');
    for (var i = 0; i < pages.length; i++) {
        var page = pages[i];
        if (page.id == 'main') {
            mainPage = page;
        }
        page.hidden = true;
    }
    if (!mainPage) {
        console.error('No main page found');
        mainPage = document.createElement('div');
    }
    pageStack.push(pageGetObjectByElement(mainPage));
    onPageStackUpdated()


});