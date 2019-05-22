/**
 * @author Samuel Larkin
 * @file plive.js
 * @brief plive client-side controller.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Centre de recherche en technologies numériques / Digital Technologies Research Centre
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2019, Sa Majeste la Reine du Chef du Canada
 * Copyright 2019, Her Majesty in Right of Canada
 */



// https://coderwall.com/p/flonoa/simple-string-format-in-javascript
/*
if (!String.prototype.format) {
   String.prototype.format = function() {
      a = this;
      for (k in arguments) {
         a = a.replace(`{${k}}`, arguments[k])
      }
      return a
   }
}
*/


// https://stackoverflow.com/a/4673436
/*
if (!String.prototype.format) {
   String.prototype.format = function() {
      const args = arguments;
      return this.replace(/{(\d+)}/g, function(match, number) {
         return typeof args[number] != 'undefined' ? args[number] : match ;
      });
   };
}
*/

if (!String.prototype.format) {
   String.prototype.format = function() {
      const args = arguments[0];  // To extract the dictionary.
      return this.replace(/{([a-zA-Z0-9_]+)}/g, function(match, number) {
         return typeof args[number] != 'undefined' ? args[number] : match ;
      });
   };
}

/*
// If you prefer not to modify String's prototype:
if (!String.format) {
   String.format = function(format) {
      var args = Array.prototype.slice.call(arguments, 1);
      return format.replace(/{(\d+)}/g, function(match, number) {
         return typeof args[number] != 'undefined' ? args[number] : match ;
      });
   };
}
*/



// https://stackoverflow.com/a/7918944
if (!String.prototype.encodeHTML) {
   String.prototype.encodeHTML = function () {
      return this.replace(/&/g, '&amp;')
         .replace(/</g, '&lt;')
         .replace(/>/g, '&gt;')
         .replace(/"/g, '&quot;')
         .replace(/'/g, '&apos;')
         .replace(/\//g, '&#x2F;');
   };
}



//Vue.use(Toasted);
Vue.use(Toasted, {
   iconPack : 'material' // set your iconPack, defaults to material. material|fontawesome
});



Vue.toasted.register('error', message => message, {
   duration : 3000,
   fullWidth : true,
   icon : 'error',
   iconPack: 'material',
   position: 'bottom-center',
   theme: 'bubble',
   type: 'error',
});



Vue.toasted.register('success', message => message, {
   duration : 3000,
   fullWidth : true,
   icon : 'done_outline',
   iconPack: 'material',
   position: 'bottom-center',
   theme: 'bubble',
   type: 'success',
});



Vue.toasted.register('info', message => message, {
   action : {
      text : 'Dismiss',
      onClick : (e, toastObject) => {
         toastObject.goAway(250);
      }
   },
   duration : 300000,
   fullWidth : true,
   icon : 'info-circle',
   iconPack: 'fontawesome',
   position: 'bottom-center',
   theme: 'bubble',
   type: 'info',
});



Vue.prototype.$body = function(method, args) {
   const method_args = Object.keys(args).reduce(function(previous, current) {
      previous += `<${current}>${String(args[current]).encodeHTML()}</${current}>`;
      return previous;
   }, '');
   return `<${method}>${method_args}</${method}>`;
}



Vue.prototype.$getBase64 = function(file) {
   return new Promise(function(resolve, reject) {
         const reader = new FileReader();
         reader.onload = function() { resolve(reader.result) };
         reader.onerror = function(error) { reject(error) };
         reader.readAsDataURL(file);
         })
      //data:[<mime type>][;charset=<charset>][;base64],<encoded data>
      //data:*/*;base64,
      //data:;base64,
      .then( function(data) {
         return data.replace(/^data:(.*\/.*)?;base64,/g, '');
      } );
}



Vue.prototype.$soap = function(soapaction, args, service_url = '/PortageLiveAPI.php') {
   // https://stackoverflow.com/questions/37693982/how-to-fetch-xml-with-fetch-api
   // https://stackoverflow.com/questions/44401177/xml-request-and-response-with-fetch
   // Response.text() returns a Promise, chain .then() to get the Promise value of .text() call.
   //
   // If you are expecting a Promise to be returned from getPostagePrice function, return fetch() call from getPostagePrice() call.
   const app = this;
   const envelope = '<?xml version="1.0" encoding="UTF-8"?> \
     <soap:Envelope \
        xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" \
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" \
        xmlns:xsd="http://www.w3.org/2001/XMLSchema"> \
        <soap:Header> \
        </soap:Header> \
        <soap:Body> \
           {body} \
         </soap:Body> \
      </soap:Envelope>'.format({'body': app.$body(soapaction, args)});

   return fetch(service_url, {
      method: 'POST',
      headers: {
         'SOAPAction': soapaction,
         'Content-Type': 'text/xml; charset=utf-8',
      },
      body: envelope,
   })
   .then(function(response) {
      /*
         if (!response.ok) {
         throw Error(response.statusText);
         }
         */
      return response.text();
   })
   .then(function(response) {
      return $.xml2json(response);
   })
   .then(function(response) {
      if (!!response.Body.Fault) {
         throw Error(`${response.Body.Fault.faultcode}: ${response.Body.Fault.faultstring}`);
      }
      return response;
   });
}



Vue.filter('doubleDigits', function (value) {
    if (typeof value !== "number") {
        return value;
    }
    const formatter = new Intl.NumberFormat('en-US', {
        style: "decimal",
        //maximumSignificantDigits: 2,
        minimumFractionDigits: 2
    });
    return formatter.format(value);
});



// https://css-tricks.com/intro-to-vue-2-components-props-slots/
// https://alligator.io/vuejs/component-communication/
Vue.component('translating', {
      props: ['is_translating'],
      template: '#vue_translating_template'
});



Vue.component('incraddsentence', {
   template: '#incraddsentence_template',
   props: ['context', 'contexts', 'document_id'],
   data: function() {
      return {
         incr_source_segment: '',
         incr_target_segment: '',
      };
   },
   methods: {
      _clear: function () {
         const app = this;
         app.incr_source_segment = '';
         app.incr_target_segment = '';
      },


      incrAddSentence: function() {
         const app = this;
         const icon = '<i class="fa fa-send"></i>';

         return app.$soap('incrAddSentence', {
               context: app.context,
               document_model_id: app.document_id,
               source_sentence: app.incr_source_segment,
               target_sentence: app.incr_target_segment,
               extra_data: '',
            })
            .then(function(response) {
               const status = response.Body.incrAddSentenceResponse.status;
               app._clear();
               // TODO: There is no feedback if the status is false.
               let myToast = app.$toasted.global.success(`Successfully added sentence pair!${icon}`);
            })
            .catch(function(err) {
               const faultstring = response.Body.Fault.faultstring;
               const faultcode = response.Body.Fault.faultcode;
               alert(`Error adding sentence pair. ${faultstring}`);
            });
      },


      is_incrAddSentence_possible: function() {
         const app = this;
         return app.context !== 'unselected'
            && app.incr_source_segment !== ''
            && app.incr_target_segment !== ''
            && app.document_id !== '';
      },
   },
});



Vue.component('incrstatus', {
   template: '#incrStatus_template',
   props: ['context', 'contexts', 'document_id'],
   data: function() {
      return {
         incrStatus_status: undefined,
      };
   },
   methods: {
      _clear: function () {
         const app = this;
         app.incrStatus_status = undefined;
      },


      incrStatus: function() {
         const app = this;
         const icon = '<i class="fa fa-cogs"></i>';
         const full_context = app.$parent._getContext();

         return app.$soap('incrStatus', {
            'context': app.context,
            'document_model_id': app.document_id,
            })
            .then(function(response) {
               const status = `full_context : ${String(response.Body.incrStatusResponse.status_description)}`;
               app.incrStatus_status = status;
               let myToast = app.$toasted.global.success(`${status} ${icon}`);
            })
            .catch(function(err) {
               let myToast = app.$toasted.global.failed(`Failed to get incremental status ${icon}`);
               alert(`Failed to get incremental status ${full_context}\n ${err}`);
            });
      },


      is_incrStatus_possible: function() {
         const app = this;
         return app.context !== 'unselected'
            && app.document_id !== undefined
            && app.document_id !== '';
      },
   },
});



Vue.component('prime', {
   template: '#prime_template',
   props: ['context', 'contexts'],
   data: function() {
      return {
         prime_mode: 'partial',
      };
   },
   methods: {
      _clear: function () {
         const app = this;
         app.prime_mode = 'partial';
      },


      is_priming_possible: function() {
         const app = this;
         return app.context !== 'unselected';
      },


      primeModel: function() {
         const app = this;
         const icon = '<i class="fa fa-tachometer"></i>';
         // context shouldn't be allowed to changed in-between invocations.
         // It should stay constant within this context.
         // Let's capture the context in case the user changes context before
         // priming is done.
         const context = app.context;

         let myToastInfo = app.$toasted.global.info(`priming ${context} ${icon}`);

         return app.$soap('primeModels', {
               'context': context,
               'PrimeMode': app.prime_mode,
            })
            .then(function(response) {
               myToastInfo.goAway(250);
               const status = String(response.Body.primeModelsResponse.status);
               if (status === 'true') {
                  let myToast = app.$toasted.global.success(`Successfully primed ${context}! ${icon}`);
               }
               else {
                  let myToast = app.$toasted.global.error(`Failed to prime ${context}! ${icon}`);
               }
            })
            .catch(function(err) {
               myToastInfo.goAway(250);
               alert(`Failed to prime context ${context} ${err}`);
            });
      },
   },
});



Vue.component('translatefile', {
   template: '#translatefile_template',
   props: ['context', 'contexts', 'document_id'],
   data: function() {
      return {
         filters: [],
         file: undefined,
         is_xml: false,
         xtags: false,
         useConfidenceEstimation: false,
         CETreshold: 0,
         translation_url: undefined,
         oov_url: undefined,
         pal_url: undefined,
         trace_url: undefined,
         translation_progress: 0,
         translate_file_error: '',
         last_translations: [],   // Create a Queue(maxSize=3)
      };
   },
   // On page loaded...
   mounted: function() {
      const app = this;
      app._createFilters();
      if (localStorage.last_translations) {
         app.last_translations = JSON.parse(localStorage.last_translations);
      }
   },
   methods: {
      _clear: function () {
         const app = this;
         app.file = undefined;
         app.is_xml = false;
         app.xtags = false;
         app.useConfidenceEstimation = false;
         app.CETreshold = 0;
         app.translation_url = undefined;
         app.oov_url = undefined;
         app.pal_url = undefined;
         app.trace_url = undefined;
         app.translation_progress = 0;
         app.translate_file_error = '';
      },


      _createFilters: function() {
         const app = this;
         app.filters = Array.apply(null, {length: 20})
                             .map(function(value, index) {
                                return index * 5 / 100;
                             });
      },


      _enqueue: function(translation) {
         const app = this;

         app.last_translations.unshift(translation);
         if (app.last_translations.length > 3) {
            app.last_translations.pop();
         }
      },


      _translateFileSuccess: function (soapResponse, method, myToastInfo) {
         const app = this;
         const icon = '<i class="fa fa-file-text"></i>';
         var response = soapResponse.Body;
         var token = response[method].token;
         // IMPORTANT we want to be able to stop the setInterval from
         // within the setInterval callback function thus we have to have
         // `watcher` in the callback's scope.
         const watcher = setInterval(
            function(monitor_token) {
               return app.$soap('translateFileStatus', { token: String(monitor_token) })
                  .then(function(response) {
                     var token = response.Body.translateFileStatusResponse.token;
                     if (token.startsWith('0 Done:')) {
                        window.clearInterval(watcher);
                        app.translation_progress = 100;
                        app.translation_url = token.replace(/^0 Done: /, '/');
                        app.pal_url = app.translation_url.replace(/[^\/]+$/, 'pal.html');
                        app.oov_url = app.translation_url.replace(/[^\/]+$/, 'oov.html');
                        let myToast = app.$toasted.global.success(`Successfully translated your file ${app.file.name}! ${icon}`);
                        myToastInfo.goAway(250);
                        app._enqueue({
                           name: app.file.name,
                           oov_url: app.oov_url,
                           pal_url: app.pal_url,
                           time: new Date().toISOString(),
                           translation_url: app.translation_url,
                        });
                     }
                     else if (token.startsWith('1')) {
                        // TODO: indicate progress.
                        // "1 In progress (0% done)"
                        const per = token.match(/1 In progress \((\d+)% done\)/);
                        if (per) {
                           app.translation_progress = per[1];
                        }
                     }
                     else if (token.startsWith('2 Failed')) {
                        myToastInfo.goAway(250);
                        window.clearInterval(watcher);
                        // 2 Failed - no sentences to translate : plive/SOAP_BtB-METEO.v2.E2F_Devoirdephilo2.docx.xliff_20180503T152059Z_mBJdDf/trace
                        const messages = token.match(/2 Failed - ([^:]+) : (.*)/);
                        // TODO: we should be checking the length of messages.
                        if (messages) {
                           app.translate_file_error = messages[1];
                           app.trace_url = `/${messages[2]}`;
                        }
                     }
                     else if (token.startsWith('3')) {
                        myToastInfo.goAway(250);
                        window.clearInterval(watcher);
                     }
                     else {
                        myToastInfo.goAway(250);
                        window.clearInterval(watcher);
                     }
                  })
                  .catch(function(err) {
                     myToastInfo.goAway(250);
                     alert(`Failed to retrieve your translation status! ${err}`);
                  });
            },
            5000,
            token);
      },


      is_ce_possible: function() {
         const app = this;
         return app.is_xml
            && app.contexts !== undefined
            && app.contexts.hasOwnProperty(app.context)
            && app.contexts[app.context].as_ce;
      },


      is_translating_a_file_possible: function() {
         const app = this;
         return app.context !== 'unselected'
            && app.file !== undefined;
      },


      prepareFile: function(evt) {
         // Inspiration: https://stackoverflow.com/questions/36280818/how-to-convert-file-to-base64-in-javascript
         // https://developer.mozilla.org/en-US/docs/Web/API/FileReader/readAsDataURL
         const app   = this;
         const files = evt.target.files;
         if (files.length === 0) {
            app.file = undefined;
            return;
         }
         const file  = files[0];

         // UI related.
         app.translation_progress = 0;
         app.translation_url      = undefined;
         app.trace_url            = undefined;
         app.translate_file_error = '';

         app.$getBase64(file)
            .then( function(data) {
               app.file = file;
               app.file.base64 = data;
               if (/\.(sdlxliff|xliff)$/i.test(file.name)) {
                  app.is_xml = true;
                  app.file.translate_method = 'translateSDLXLIFF';
               }
               else if (/\.tmx$/i.test(file.name)) {
                  app.is_xml = true;
                  app.file.translate_method = 'translateTMX';
               }
               else if (/\.txt$/i.test(file.name)) {
                  app.is_xml = false;
                  app.file.translate_method = 'translatePlainText';
               }
               else {
                  app.is_xml = false;
                  app.file.translate_method = 'translatePlainText';
               }
            })
            .catch( function(error) {
               alert("Error converting your file to base64!");
            } );
      },


      translateFile: function(evt) {
         const app = this;
         const icon = '<i class="fa fa-file-text"></i>';
         const translate_method = app.file.translate_method;
         const data = {
            ContentsBase64: app.file.base64,
            Filename: app.file.name,
            context: app.$parent._getContext(),
            useCE: app.useConfidenceEstimation,
            xtags: app.xtags,
         };

         if (app.is_xml) {
            data.CETreshold = app.CETreshold;
         }

         // UI related.
         app.translation_progress = 0;
         app.translation_url      = undefined;
         app.trace_url            = undefined;
         app.translate_file_error = '';

         let myToastInfo = app.$toasted.global.info(`${app.$parent.translating_animation} ${data.Filename} ${icon}`);

         return app.$soap(translate_method, data)
            .then(function(response) {
               app._translateFileSuccess(response, `${translate_method}Response`, myToastInfo);
            })
            .catch(function(err) {
               myToastInfo.goAway(250);
               alert(`Failed to translate your file! ${soapResponse.toJSON()}`);
            });
      },
   },  // methods
   watch: {
      last_translations: {
         handler: function(val, oldVal) {
            localStorage.last_translations = JSON.stringify(val);
         },
         deep: true,
      },
   },
});



Vue.component('translatetext', {
   template: '#translatetext_template',
   props: ['context', 'contexts', 'document_id'],
   data: function() {
      return {
         source: '',
         xtags: false,
         pretokenized: false,
         enable_phrase_table_debug: false,
         translation: '',
         newline: 'p',
         oov_url: undefined,
         pal_url: undefined,
      };
   },
   methods: {
      _clear: function () {
         const app = this;
         app.source = '';
         app.xtags = false;
         app.pretokenized = false;
         app.enable_phrase_table_debug = false;
         app.translation = '';
         app.newline = 'p';
         app.oov_url = undefined;
         app.pal_url = undefined;
      },

      is_translating_possible: function() {
         const app = this;
         return app.context !== 'unselected'
            && app.source !== undefined
            && app.source !== '';
      },


      add_newline: function() {
         const app = this;
         app.source += '\n';
      },


      translate: function() {
         const app = this;
         const icon = '<i class="fa fa-keyboard-o"></i>';
         app.translation = '';
         const is_incremental = app.contexts[app.context].is_incremental;
         if (app.document_id !== undefined && app.document_id !== '' && !is_incremental) {
            alert(`${app.context} does not support incremental.  Please select another system.`);
            return;
         }

         let myToastInfo = app.$toasted.global.info(`${app.$parent.translating_animation}${icon}`);
         app.oov_url = undefined;
         app.pal_url = undefined;

         return app.$soap('translate', {
               srcString: app.source,
               context: app.$parent._getContext(),
               newline: app.newline,
               xtags: app.xtags,
               useCE: false,
            })
            .then(function(response) {
               myToastInfo.goAway(250);
               app.translation = response.Body.translateResponse.Result;
               let workdir = response.Body.translateResponse.workdir;
               app.oov_url = `${workdir}/oov.html`;
               app.pal_url = `${workdir}/pal.html`;

               let myToast = app.$toasted.global.success(`Successfully translated your text! ${icon}`);
            })
            .catch(function(err) {
               myToastInfo.goAway(250);
               alert(`Failed to translate your sentences! ${err}`);
               let myToast = app.$toasted.global.error(`Failed to translate your text! ${icon}`);
            });
      },


      translate_using_REST_API: function () {
         const app = this;
         // https://developers.google.com/web/updates/2015/03/introduction-to-fetch
         // https://developer.mozilla.org/en-US/docs/Web/API/WindowOrWorkerGlobalScope/fetch
         app.translation_segments = null;
         const request = {
            'options': {
               'sent_split': new Boolean(true),
            },
            'segments': [
               app.source_segment
            ]};
         fetch('/translate', {
            method: 'POST',
            headers: {
               'Accept': 'application/json',
               'Content-Type': 'application/json',
               },
            body: JSON.stringify(request),
            })
         .then(function(response) { response.json() } )
         .then(function(translations) {
            app.translation_segments = translations;
            console.log(translations);})
         .catch(function(err) { console.error(err) } )
      },
   },
});



var plive_app = new Vue({
   el: '#plive_app',

   data: {
      // Load up the template from the UI.
      translating_animation: document.getElementById('translating_template').text || 'translating...',
      contexts: [],
      context: 'unselected',
      document_id: '',
      version: '',
   },

   // On page loaded...
   mounted: function() {
      const app = this;
      app.getAllContexts();
      app.getVersion();

      if (localStorage.document_id) {
         app.document_id = localStorage.document_id;
      }

      if (false) {
         // This is useful for debugging vue-toasted.
         let myToastFailed = app.$toasted.global.error('<i class="fa fa-car"></i>My custom error message');
         let myToastSuccess = app.$toasted.global.success('Successfully translate your file! <i class="fa fa-file"></i><i class="fa fa-file-text"></i><i class="fa fa-edit"></i><i class="fa fa-keyboard-o"></i> <i class="fa fa-pencil"></i>', {duration: 10000});
         let myToastInfo = app.$toasted.info(app.translating_animation, {
            duration: 100000,
            action : {
               text : 'Dismiss',
               onClick : (e, toastObject) => {
                  toastObject.goAway(0);
               }
            },
         });
      }

      if (false) {
         /*
         <script src="https://cdn.jsdelivr.net/npm/tinysoap@1.0.2/tinysoap-browser-min.js"></script>
         <script type="text/javascript" src="tinysoap-browser.js"></script>
         */
         const wsdl = '/PortageLiveAPI.wsdl';
         tinysoap.createClient(wsdl, function(err, client) {
            // First function call test.
            client.getAllContexts(
               {
                  'verbose':false,
                  'json':true
               },
               function(err, result) {
                  var c = JSON.parse(result['contexts']);
                  console.log(c)
               });
            // Second function call test.
            client.getVersion(
               {},
               function(err, result) {
                  var c = result['contexts'];
                  console.log(c)
               });
         });
      }
   },


   methods: {
      _getContext: function() {
         const app = this;
         var context = app.context;
         if (app.document_id !== undefined && app.document_id !== '') {
            context = `${context}/${app.document_id}`;
         }
         return context;
      },


      clearForm: function() {
         const app = this;

         app.context = 'unselected';
         app.document_id = '';

         // TODO: apply clear() on all children.
         app.$children.forEach(function(e) {
            e._clear();
         });
      },


      getAllContexts: function() {
         const app = this;

         return app.$soap('getAllContexts', {'verbose': false, 'json': true})
            .then(function(response) {
               // We've asked for a json replied which is embeded in the xml response.
               const jsonContexts = JSON.parse(response.Body.getAllContextsResponse.contexts);
               const contexts = jsonContexts.contexts.reduce(function(acc, cur, i) {
                     acc[cur.name] = cur;
                     return acc;
                  },
                  {});
               app.contexts = contexts;
               app.context  = 'unselected';
               if (localStorage.context && app.contexts.hasOwnProperty(localStorage.context)) {
                  app.context = localStorage.context;
               }
            })
            .catch(function(err) {
               alert(`Error fetching available contexts/systems from the server. ${err}`);
            });
      },


      getVersion: function() {
         // https://stackoverflow.com/questions/37693982/how-to-fetch-xml-with-fetch-api
         // https://stackoverflow.com/questions/44401177/xml-request-and-response-with-fetch
         // Response.text() returns a Promise, chain .then() to get the Promise value of .text() call.
         //
         // If you are expecting a Promise to be returned from getPostagePrice function, return fetch() call from getPostagePrice() call.
         const app = this;

         return app.$soap('getVersion', {})
            .then(function(response) {
               app.version = String(response.Body.getVersionResponse.version);
               console.log(app.version);
            })
            .catch(function(err) {
               app.version = "Failed to retrieve Portage's version";
               alert(`Failed to get Portage's version. ${err}`);
            });
      },
   },  // methods


   watch: {
      'context': function (val, oldVal) {
         console.log(`Changing context from ${oldVal} to ${val}`);
         localStorage.setItem('context', val);
      },
      'document_id': function (val, oldVal) {
         console.log(`Changing document_id from ${oldVal} to ${val}`);
         localStorage.setItem('document_id', val);
      },
   },
});
