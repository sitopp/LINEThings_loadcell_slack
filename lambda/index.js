'use strict';

// lambdaの環境変数に以下を設定すること
// ACCESS_TOKEN
// CHANNEL_SECRET
// SLACK_WEBHOOK_URL 
//    https://qiita.com/namutaka/items/233a83100c94af033575 のIncoming webhooks タイプのWebhooksのURLを作成して書く


const line = require('@line/bot-sdk');
const crypto = require('crypto');
const client = new line.Client({ channelAccessToken: process.env.ACCESS_TOKEN });
const messageObject = require('./message_object');
var exec = require('exec');
var exec = require('child_process').exec;
var slack_webhook_url = process.env.SLACK_WEBHOOK_URL;

exports.handler = function (event, context) {
    let signature = crypto.createHmac('sha256', process.env.CHANNEL_SECRET).update(event.body).digest('base64');
    let checkHeader = (event.headers || {})['X-Line-Signature'];
    let body = JSON.parse(event.body);
    if (signature === checkHeader) {
        if (body.events[0].replyToken === '00000000000000000000000000000000') { //接続確認エラー回避
            let lambdaResponse = {
                statusCode: 200,
                headers: { "X-Line-Status": "OK" },
                body: '{"result":"connect check"}'
            };
            console.log('L.30 LINE developers consoleからの疎通確認'); //デバッグ
            context.succeed(lambdaResponse);

        } else {
            //共通処理
            var replyToken = body.events[0].replyToken;
            let userID = body.events[0].source.userId;

            //デバッグ
            console.log('L.39 body=' + JSON.stringify(body));
            console.log('L.45 body.events[0].type=' + body.events[0].type);
            console.log('L.41 replyToken=' + replyToken);
            console.log('L.60 body.events[0].source.userId=' + userID);

            //イベントタイプ毎に処理を分ける
            if (body.events[0].type == 'things') {

                //slackにメッセージ
                let postData = {
                    'channel': '#general',
                    'username': 'webhookbot',
                    'text': 'オフィスのコーラが不足してしまいました。補充しないとです。　',
                    'icon_emoji': ':ghost:'
                }
                slack_call(postData).catch(e => console.log(e));

                //応答内容編集                    
                const msg = '冷蔵庫のコーラがストック不足です。' +
                    '山本さんに連絡しますので、いまどこの冷蔵庫の前なのか、場所を教えてね。';
                const replyMessage = messageObject.messageObject_quickreply(msg);
                console.log('L.52 replyMessage=' + JSON.stringify(replyMessage)); //デバッグ
                client.replyMessage(replyToken, replyMessage)
                    .then((response) => {
                        let lambdaResponse = messageObject.lambdaResponse();
                        context.succeed(lambdaResponse);
                    }).catch((err) => console.log(err));

            } else if (body.events[0].type == 'message' && body.events[0].message.type == 'location') {
                //クイックリプライで位置情報が送られてきた場合はこちら

                //応答内容編集                    
                const msg = '冷蔵庫の位置情報を山本さんに連絡しました。ご協力ありがとうございました！';
                const replyMessage = messageObject.messageObject_text(msg);
                console.log('L.80 replyMessage=' + JSON.stringify(replyMessage)); //デバッグ
                var map_url = "https://www.google.co.jp/maps/@" +
                    body.events[0].message.latitude + ',' +
                    body.events[0].message.longitude + ',';

                //slackにメッセージ
                //slackにメッセージ
                let postData = {
                    'channel': '#general',
                    'username': 'webhookbot',
                    'text': 'オフィスのコーラが不足してしまいました。補充しないとです。　' + map_url,
                    'icon_emoji': ':ghost:'
                }
                slack_call(postData).catch(e => console.log(e));

                client.replyMessage(replyToken, replyMessage)
                    .then((response) => {
                        let lambdaResponse = messageObject.lambdaResponse();
                        context.succeed(lambdaResponse);
                    }).catch((err) => console.log(err));
            } else {  //クイックリプライでもThingsでもない時

                //応答内容編集                    
                const msg = '冷蔵庫のコーラがストック不足です。' +
                    '山本さんに連絡しますので、いまどこの冷蔵庫の前なのか、場所を教えてね。';
                const replyMessage = messageObject.messageObject_quickreply(msg);
                console.log('L.52 replyMessage=' + replyMessage); //デバッグ
                client.replyMessage(replyToken, replyMessage)
                    .then((response) => {
                        let lambdaResponse = messageObject.lambdaResponse();
                        context.succeed(lambdaResponse);
                    }).catch((err) => console.log(err));


                console.log('L.52 replyMessage=' + replyMessage); //デバッグ


                client.replyMessage(replyToken, replyMessage)
                    .then((response) => {
                        let lambdaResponse = messageObject.lambdaResponse();
                        context.succeed(lambdaResponse);
                    }).catch((err) => console.log(err));
            }

        }

    } else {
        console.log('署名認証エラー');
    }
};


async function slack_call(postData) {
    //なんかうまく動かないので調整中 2019.12.11
    // var msg = 'curl - X POST--data - urlencode " + JSON.stringify(postData) + " -H 'Content-Type: application/json' " + slack_webhook_url;
    var msg = 'curl - X POST--data - urlencode "payload={\"channel\": \"#cola-yamamoto\", \"username\": \"webhookbot\", \"text\": \"山本さんのコーラが残りわずかです！！。\", \"icon_emoji\": \":ghost:\"}" ' + slack_webhook_url;

    console.log('L.139 msg: ' + msg);

    await exec(msg, function (error, stdout, stderr) {
        console.log('L.199 stdout: ' + stdout);
        console.log('L.200 stderr: ' + stderr);
        if (error !== null) {
            console.log('L.202 exec error: ' + error);
        }
    });
}
