#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <json-c/json.h>

#define URL0 "https://tt.chadsoft.co.uk/leaderboard/"
#define URL1 ".json?limit=1&vehicle="

#define VEHICLE_COUNT 36

static const char *const vehicle_names[] = {
        "Standard Kart S",
        "Standard Kart M",
        "Standard Kart L",
        "Booster Seat",
        "Classic Dragster",
        "Offroader",
        "Mini Beast",
        "Wild Wing",
        "Flame Flyer",
        "Cheep Charger",
        "Super Blooper",
        "Piranha Prowler",
        "Tiny Titan",
        "Daytripper",
        "Jetsetter",
        "Blue Falcon",
        "Sprinter",
        "Honeycoupe",
        "Standard Bike S",
        "Standard Bike M",
        "Standard Bike L",
        "Bullet Bike",
        "Mach Bike",
        "Flame Runner",
        "Bit Bike",
        "Sugarscoot",
        "Wario Bike",
        "Quacker",
        "Zip Zip",
        "Shooting Star",
        "Magikruiser",
        "Sneakster",
        "Spear",
        "Jet Bubble",
        "Dolphin Dasher",
        "Phantom",
};

#define TRACK_COUNT 4
#define TRACK_OFFSET 4

static const char *const track_ids[] = {
        "08/1AE1A7D894960B38E09E7494373378D87305A163/00", // LC
        "01/90720A7D57A7C76E2347782F6BDE5D22342FB7DD/00", // MMM
        "02/0E380357AFFCFD8722329994885699D9927F8276/02", // MG no-sc
        "04/1896AEA49617A571C66FF778D8F2ABBE9E5D7479/02", // TF no-sc
        "00/7752BB51EDBC4A95377C0A05B0E0DA1503786625/00", // MC normal
        "05/E4BF364CB0C5899907585D731621CA930A4EF85C/02", // CM no-sc
        "06/B02ED72E00B400647BDA6845BE387C47D251F9D1/00", // DKSC
        "07/D1A453B43D6920A78565E65A4597E353B177ABD0/00", // WGM normal
        "09/72D0241C75BE4A5EBD242B9D8D89B1D6FD56BE8F/00", // DC
        "0F/52F01AE3AED1E0FA4C7459A648494863E83A548C/00", // KC
        "0B/48EBD9D64413C2B98D2B92E5EFC9B15ECD76FEE6/00", // MT normal
        "03/ACC0883AE0CE7879C6EFBA20CFE5B5909BF7841B/02", // GV no-sc
        "0E/38486C4F706395772BD988C1AC5FA30D27CAE098/00", // DDR
        "0A/B13C515475D7DA207DFD5BADD886986147B906FF/00", // MH
        "0C/B9821B14A89381F9C015669353CB24D7DB1BB25D/02", // BC no-sc
        "0D/FFE518915E5FAAA889057C8A3D3E439868574508/00", // RR normal
        "10/8014488A60F4428EEF52D01F8C5861CA9565E1CA/00", // rPB normal
        "14/8C854B087417A92425110CC71E23C944D6997806/00", // rYF
        "19/071D697C4DDB66D3B210F36C7BF878502E79845B/00", // rGV2 normal
        "1A/49514E8F74FEA50E77273C0297086D67E58123E8/00", // rMR
        "1B/BA9BCFB3731A6CB17DBA219A8D37EA4D52332256/00", // rSL normal
        "1F/E8ED31605CC7D6660691998F024EED6BA8B4A33F/00", // rSGB normal
        "17/BC038E163D21D9A1181B60CF90B4D03EFAD9E0C5/00", // rDS
        "12/418099824AF6BF1CD7F8BB44F61E3A9CC3007DAE/00", // rWS normal
        "15/4EC538065FDC8ACF49674300CBDEC5B80CC05A0D/02", // rDH no-sc
        "1E/A4BEA41BE83D816F793F3FAD97D268F71AD99BF9/02", // rBC3 no-sc
        "1D/692D566B05434D8C66A55BDFF486698E0FC96095/02", // rDKJP no-sc
        "11/1941A29AD2E7B7BBA8A29E6440C95EF5CF76B01D/00", // rMC
        "18/077111B996E5C4F47D20EC29C2938504B53A8E76/00", // rMC3
        "16/F9A62BEF04CC8F499633E4023ACC7675A92771F0/00", // rPG
        "13/B036864CF0016BE0581449EF29FB52B2E58D78A4/02", // rDKM no-sc
        "1C/15B303B288F4707E5D0AF28367C8CE51CDEAB490/02", // rBC no-sc
};

#define REQUEST_COUNT (VEHICLE_COUNT * TRACK_COUNT)

struct response {
        size_t size;
        char *buf;
};

static size_t write_cb(char *data, size_t size, size_t nmemb, void *userp) {
        size_t real_size = size * nmemb;
        struct response *response = userp;

        char *new_buf = realloc(response->buf, response->size + real_size + 1);
        if (!new_buf) {
                return 0;
        }

        response->buf = new_buf;
        memcpy(response->buf + response->size, data, real_size);
        response->size += real_size;

        return real_size;
}

static int fetch_database(struct response responses[REQUEST_COUNT]) {
        int ret = 1;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        CURLM *cm = curl_multi_init();
        curl_multi_setopt(cm, CURLMOPT_MAX_TOTAL_CONNECTIONS, 8);

        CURL *ehs[REQUEST_COUNT] = { 0 };
        for (size_t i = 0; i < VEHICLE_COUNT; i++) {
                for (size_t j = 0; j < TRACK_COUNT; j++) {
                        size_t url_size = strlen(URL0) + strlen(track_ids[j + TRACK_OFFSET]) + strlen(URL1) + 2;
                        char *url = malloc(url_size + 1);
                        if (!url) {
                                goto cleanup;
                        }
                        if ((size_t)snprintf(url, url_size + 1, URL0 "%s" URL1 "%02zu", track_ids[j + TRACK_OFFSET], i) != url_size) {
                                goto cleanup;
                        }

                        size_t req_idx = TRACK_COUNT * i + j;
                        ehs[req_idx] = curl_easy_init();
                        curl_easy_setopt(ehs[req_idx], CURLOPT_URL, url);
                        curl_easy_setopt(ehs[req_idx], CURLOPT_WRITEFUNCTION, write_cb);
                        curl_easy_setopt(ehs[req_idx], CURLOPT_WRITEDATA, responses + req_idx);
                        curl_easy_setopt(ehs[req_idx], CURLOPT_FAILONERROR, 1);
                        curl_multi_add_handle(cm, ehs[req_idx]);

                        free(url);
                }
        }

        int still_alive;
        do {
                if (curl_multi_perform(cm, &still_alive)) {
                        goto cleanup;
                }
                fprintf(stderr, "\r%u / %u", REQUEST_COUNT - still_alive, REQUEST_COUNT);
                CURLMsg *msg;
                int msgs_left;
                while ((msg = curl_multi_info_read(cm, &msgs_left))) {
                        if (msg->msg != CURLMSG_DONE) {
                                goto cleanup;
                        } else if (msg->data.result) {
                                CURL *eh = msg->easy_handle;
                                curl_multi_remove_handle(cm, eh);
                                curl_multi_add_handle(cm, eh);
                                still_alive++;
                        }
                }
                if (curl_multi_poll(cm, NULL, 0, 1000, NULL)) {
                        goto cleanup;
                }
        } while (still_alive);
        fprintf(stderr, "\n");

        for (size_t i = 0; i < REQUEST_COUNT; i++) {
                responses[i].buf[responses[i].size] = '\0';
        }

        ret = 0;

cleanup:
        for (size_t i = 0; i < REQUEST_COUNT; i++) {
                curl_multi_remove_handle(cm, ehs[i]);
                curl_easy_cleanup(ehs[i]);
        }
        curl_multi_cleanup(cm);
        curl_global_cleanup();

        return ret;
}

static void print_vehicle_times(size_t i, struct response *responses) {
        printf("%s", vehicle_names[i]);

        for (size_t j = 0; j < TRACK_COUNT; j++) {
                printf(",");

                struct json_object *main = json_tokener_parse(responses[j].buf);
                if (!main) {
                        continue;
                }

                struct json_object *ghosts = json_object_object_get(main, "ghosts");
                if (!ghosts) {
                        json_object_put(main);
                        continue;
                }

                struct json_object *ghost = json_object_array_get_idx(ghosts, 0);
                if (!ghost) {
                        json_object_put(main);
                        continue;
                }

                struct json_object *time = json_object_object_get(ghost, "finishTimeSimple");
                if (!time) {
                        json_object_put(main);
                        continue;
                }

                printf("%s", json_object_get_string(time));

                json_object_put(main);
        }

        printf("\n");
}

int main(void) {
        int ret = 1;

        struct response responses[REQUEST_COUNT] = { 0 };
        if (fetch_database(responses)) {
                goto cleanup;
        }

        for (size_t i = 0; i < VEHICLE_COUNT; i++) {
                print_vehicle_times(i, responses + TRACK_COUNT * i);
        }

        ret = 0;

cleanup:
        for (size_t i = 0; i < REQUEST_COUNT; i++) {
                free(responses[i].buf);
        }

        return ret;
}
