'use strict';


var apiList = ['{"msg": ""}',
               '{"msg": "param_set", "data": {"flash_screen": false, "keyboard_layout": 155,"knock_enabled": false, "knock_sensitivity": 1,"lock_timeout": 0,"lock_timeout_enabled": false,"offline_mode": false,"screen_brightness": 51, "screensaver": false,"tutorial_enabled": false,"user_interaction_timeout": 0,"user_request_cancel": false}}',
               '{ "msg": "start_memorymgmt" }',
               '{ "msg": "exit_memorymgmt" }',
               '{"msg": "get_credential", "data": {"service": "sub.example.com", "fallback_service": "example.com", "login": "user@example.com", "request_id": "123"}}',
               '{"msg": "set_credential", "data": {"service": "example.com", "login": "user@example.com", "password": "pwd", "description": "added by API"}}',
               '{"msg": "get_random_numbers"}',
               '{"msg": "cancel_request", "data": {"request_id": "123"}}',
               '{"msg": "get_data_node", "data": {"service": "data example", "fallback_service": "data example2", "request_id": "123"}}',
               '{"msg": "set_data_node", "data": {"service": "data example", "request_id": "123", "node_data": "<base64_encoded_data>"}}',
               '{"msg": "show_app"}',
               '{"msg": "start_memcheck"}',
               '{"msg": "get_application_id"}',
               '{"msg": "credential_exists", "data": {"service": "example.com"}}',
               '{"msg": "data_node_exists", "data": {"service": "data example"}}'
]

function popuplateApiList() {
    var i = 0;
    $('#api_list').append($('<option />').val(i++).html('Custom request'));
    for (var c = 1;c < apiList.length;c++) {
        var s = JSON.parse(apiList[c]).msg;
        $('#api_list').append($('<option />').val(i++).html(s));
    }

    $('#api_list').change(function() {
        var j = $('#api_list option:selected').val();
        var s = apiList[parseInt(j)];
        $('#message').val(JSON.stringify(JSON.parse(s), null, '    '));
    });

    $('#api_list').change();
}

var ws = new Object();
var loginfos = [];

function addLog(o, sending) {

    sending = typeof sending !== 'undefined' ? sending : false;

    // <p><span class="glyphicon glyphicon-import"></span>log test</p>
    // <p class="pselected"><span class="glyphicon glyphicon-import"></span>log test2</p>
    var id = loginfos.length;
    var newItem = $('<p></p>');
    loginfos.push(o);

    $('.log').append(newItem);
    if (o.type == 'connected') {
        newItem.append($('<span class="glyphicon glyphicon-ok">'));
        newItem.append('Connected');
    }
    else if (o.type == 'disconnected') {
        newItem.append($('<span class="glyphicon glyphicon-remove">'));
        newItem.append('Disconnected');
    }
    else if (o.type == 'error') {
        newItem.append($('<span class="glyphicon glyphicon-remove-sign">'));
        newItem.append(o.type);
    }
    else if (sending) {
        newItem.append($('<span class="glyphicon glyphicon-export">'));
        if (o.type == '')
            newItem.append('Empty "msg"');
        else
            newItem.append(o.type);
    }
    else {
        newItem.append($('<span class="glyphicon glyphicon-import">'));
        if (o.type == '')
            newItem.append('Empty "msg"');
        else
            newItem.append(o.type);
    }

    $('.badge').html(loginfos.length);

    newItem.on('click', function () {
        var data = loginfos[id];
        var txt;
        try {
            txt = JSON.stringify(JSON.parse(data.json), null, '    ');
        }
        catch(ex) {
            txt = data.json;
        }

        $('#answer').html(txt);

        $('pre code').each(function(i, block) {
          hljs.highlightBlock(block);
        });
    });
}

function clearLog() {
    loginfos = [];
    $('.log').html('');
    $('.badge').html('0');
}

$(document).ready(function() {

    popuplateApiList();

    //$('#urlinput').val('ws://' + window.location.host + '/api');

    $('#btdisconnect').on('click', function () {
        ws.close();
    });

    $('#btsend').on('click', function () {

        var m = $('#message').val();

        if ('readyState' in ws &&
             ws.readyState == ws.OPEN) {
            ws.send(m);
        }
        else {
            console.log('sending error');
            addLog({
                type: 'error',
                msg_id: '',
                json: 'Websocket connection closed'
            });
            return;
        }

        var json = JSON.parse(m);

        var id = '';
        var msg = 'UNKNOWN!!';
        if (json.hasOwnProperty('msg_id'))
            id = json.msg_id;
        if (json.hasOwnProperty('msg'))
            msg = json.msg;

        addLog({
            type: msg,
            msg_id: id,
            json: m
        }, true);
    });

    $('#btclear').on('click', function () {
        clearLog();
    });

    $('#btconnect').on('click', function () {
        ws = new WebSocket($('#urlinput').val());
        ws.onopen = function(evt) {
            $('#status_con').removeClass('label-danger');
            $('#status_con').addClass('label-success');
            $('#status_con').html('Connected');

            addLog({
                type: 'connected',
                msg_id: '',
                json: 'OPEN'
            });
        };
        ws.onclose = function(evt) {
            $('#status_con').removeClass('label-success');
            $('#status_con').addClass('label-danger');
            $('#status_con').html('Disconnected');

            addLog({
                type: 'disconnected',
                msg_id: '',
                json: 'CLOSED'
            });
        };
        ws.onmessage = function(evt) {
            console.log(evt.data);

            var json = JSON.parse(evt.data);

            var id = '';
            var msg = 'UNKNOWN!!';
            if (json.hasOwnProperty('msg_id'))
                id = json.msg_id;
            if (json.hasOwnProperty('msg'))
                msg = json.msg;

            addLog({
                type: msg,
                msg_id: id,
                json: evt.data
            });
        };
        ws.onerror = function(evt) {
            $('#status_con').removeClass('label-success');
            $('#status_con').addClass('label-danger');
            $('#status_con').html('Disconnected');

            addLog({
                type: 'error',
                msg_id: '',
                json: evt.data
            });
        };
    });
});
