module.exports = {
    messageObject_text: (returnText) => {
        var messageObject_text = {
            type: 'text',
            text: returnText
        }
        return messageObject_text
    },
    lambdaResponse: () => {
        let lambdaResponse = {
            statusCode: 200,
            headers: { "X-Line-Status": "OK" },
            body: '{"result":"completed"}'
        }
        return lambdaResponse
    },
    messageObject_quickreply: (returnText) => {
        var messageObject_text = {
            "type": "text",
            "text": returnText,
            "quickReply": {
                "items": [
                    {
                        "type": "action",
                        "action": {
                            "type": "location",
                            "label": "位置情報を送る"
                        }
                    }
                ]
            }
        }
        return messageObject_text
    },

}   