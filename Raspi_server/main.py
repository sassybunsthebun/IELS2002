# Kilder brukt i dette programmet: 
# Paho-MQTT, V. 1.6.1, https://pypi.org/project/paho-mqtt/1.6.1/, 6/12/2024
# EnTur GraphQL IDE https://api.entur.io/graphql-explorer/vehicles?query=%7B%0A%20%20vehicles%28codespaceId%3A%20%22ATB%22%2C%2
# 0lineRef%3A%20%22ATB%3ALine%3A2_3%22%2C%20vehicleId%3A%20%22%22%29%20%7B%0A%20%20%20%20line%20%7B%0A%20%20%20%20%20%20lineRef
# %0A%20%20%20%20%20%20lineName%0A%20%20%20%20%7D%0A%20%20%20%20lastUpdated%0A%20%20%20%20location%20%7B%0A%20%20%20%20%20%20la
# titude%0A%20%20%20%20%20%20longitude%0A%20%20%20%20%7D%0A%20%20%20%20direction%0A%20%20%20%20vehicleId%0A%20%20%7D%0A%7D%0A&va
# riables=%7B%7D
# Hovedprosjekt 2023, Smart City - Smarte, dynamiske bomstasjoner, Magnus Frafjord, Jonatan Franzen, Njål Bruheim, Iver Ødegård, Elland Øian, 17/6/2024
# API-examples, EnTur, https://github.com/entur/api-examples, 2018

import paho.mqtt.client as mqtt
import json
import numpy as np
import time
import requests
import socket
import numpy as np

# Hovedflagg for å indikere tilstand:
idle = True
plan = False
search = False
pre_follow = False
sort_bus = False
follow = False

# Lister for lagring av bussretning:
Outbound = []
Inbound = []

# timer variabler:
intervall = 4
prevmillis = 0
prev_lock_on_distance = 0

# Koordinater:
lengdegrad = 0 
breddegrad = 0 

# Range vi ønsker å lete etter busser:
rekkevidde = 700

# Retningsvariabel:
dir = None # = buss_retning_tuple[1]

# ---------------------------- API/GraphQl variabler/funksjoner -------------------------------------#
# API endpoint
GRAPHQL_ENDPOINT = "https://api.entur.io/realtime/v2/vehicles/graphql"

# Header inneholder informasjon om klienten som sender requests til API'et
HEADERS = {
    'Accept': 'application/json',
    'Content-Type': 'application/json',
    'User-Agent': 'Njaal_NTNU' + socket.gethostname(),
    'ET-Client-Name': 'Njaal_NTNU' + socket.gethostname(),
    'ET-Client-ID': socket.gethostname()
}

# Query for å få tak data til alle bussene på linje 3
query = '''{
  vehicles(codespaceId: "ATB", lineRef: "ATB:Line:2_3") {
    line {
      lineRef
      lineName
    }
    lastUpdated
    location {
      latitude
      longitude
    }
    direction
    originName
    vehicleId
  }
}'''

# Denne funksjonen sender query og returnerer det som json;
def sendGraphqlQuery(query):
    data = {'query': query}
    response = requests.post(GRAPHQL_ENDPOINT, json=data, headers=HEADERS)
    return response.json()

# Denne funksjonen sender query og returnerer det som json;
def lock_on(IdQuery):
    IdData = {'query': IdQuery}
    response = requests.post(GRAPHQL_ENDPOINT, json=IdData, headers=HEADERS)
    return response.json()

# ----------------------------MQTT varibler/funksjoner-------------------------#

# Konfigurerer innloggingsinformasjon og broker adresse for MQTT
broker_address = "84.214.173.115"
port = 1883
username = "Njaaaaaaaaaaal"
password = None

stopp_lengdegrad = None
stopp_breddegrad = None

# Parameter fra mqtt
GPS_breddegrad = 0
GPS_lengdegrad = 0
start_index = 0

# Indexen vi skal til
destinasjon_index = 0

# Meldingsteller for å indikere når vi går over i neste fase:
message_counter = 0

# global payload til å lagre motatte verdier
global_payload = [0,0,0]

# Callback for når klienten kobler seg til broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"Tilkobling var vellykket {rc}")
         # Abonnerer på topic "esp32/output"
        client.subscribe("testTopic")
    else:
        print(f"Mislykket tilkobling {rc}")
        
# Callback for når en melding er motatt fra brokeren
def on_message(client, userdata, msg):
    global global_payload
    payload = msg.payload.decode('utf-8')
    print(f"Motatt melding: '{payload}' på topic: '{msg.topic}'")
    # Splitter meldingen og gjør den om til en liste
    global_payload = payload.split(',')
    print(global_payload)
    global message_counter
    message_counter += 1
    print(f'Message_counter = {message_counter}')
    return global_payload

def on_publish(client, userdata, mid):
    print(f'Følgende melding ble publisert:{mid}')
    
# Funksjon for å publisere en melding på mqtt
def publish_message(broker, port, pubtopic, message):
    client = mqtt.Client()
    client.connect(broker, port, 60)
    client.publish(pubtopic, message)
    client.disconnect()

# Instans av mqtt_client
client = mqtt.Client("Hovedprosjekt_main")

client.username_pw_set(username, password)

# Tildeler de ulike callback-funksjonene
client.on_connect = on_connect
client.on_message = on_message
client.on_publish = on_publish

# Kobler seg til broker
client.connect(broker_address, port, 60)

# --------------------------------------- Andre funksjoner -------------------------------#

# åpner JSON fil med bussene:
with open("koordinater.json", "r") as file:
    koordinat_data = json.load(file)

# Funksjon som regner ut distanse mellom to punkter
def distanse_kalk(bredde1, lengde1, bredde2, lengde2):
    return 111100*(np.sqrt((float(bredde1)-float(bredde2))**2 + (float(lengde1)-float(lengde2))**2))

lengde = len(koordinat_data["Busstopp"])

# Funksjon som finner nærmeste stopp basert på GPS posisjon
# returnerer stoppnavn til nærmeste stopp og indexen dette stoppet har i dictionaryet
def finn_stopp (GPS_breddegrad, GPS_lengdegrad):
    naermest = None
    min_dist = 100
    for i in range(lengde):
        stopp_breddegrad = float(koordinat_data["Busstopp"][i]["Koordinater"][0])
        stopp_lengdegrad = float(koordinat_data["Busstopp"][i]["Koordinater"][1])
        #print(f'Breddegrad{i} = {stopp_breddegrad} og lengdegrad{i} = {stopp_lengdegrad}')
        distanse = distanse_kalk(GPS_breddegrad, GPS_lengdegrad, stopp_breddegrad, stopp_lengdegrad )
        #print(distanse)
        start_index = i
        if distanse < min_dist:
            naermest = koordinat_data["Busstopp"][i]["Stoppnavn"]
            start_index = i
    return naermest, start_index

# Beregner retningen på bussen basert på relasjonen mellom stopp og start index
def retning(utgangspunkt):
    temp_val = int(utgangspunkt[1]) - int(destinasjon_index)-4
    print(f'Retningsverdi er: {temp_val}')
    destinasjons_stopp = koordinat_data["Busstopp"][destinasjon_index]["Stoppnavn"]
    if temp_val > 0:
        global dir
        dir = "Outbound"
    else:
        dir = "Inbound"
    return temp_val,dir,destinasjons_stopp

#-----------------------------------------STATES--------------------------------#

# Idle tilstand
while idle:
    # Kaller på mqtt client loopen for å kunne motta og sende meldinger, timeout på 1 sekund siden denne loopen er blokkerende
    client.loop(timeout=1.0, max_packets=None)
    print("Idle modus, send destinasjon og startpunkt...")
    # Tømmer listene før neste iterasjon
    Outbound.clear()
    Inbound.clear()
    if message_counter > 0:
        plan = True
        idle = False

# Plan tilstand:
while plan:
    # Kaller på mqtt client loopen for å kunne motta og sende meldinger
    client.loop(timeout=1.0, max_packets=None)
    # Finner stoppet som er nærmest brukerens posisjon
    if global_payload[1] != 0 and global_payload[1] != 1:
        destinasjon_index = int(global_payload[0])
        GPS_breddegrad = global_payload[1]
        GPS_lengdegrad = global_payload[2]
        utgangspunkt = finn_stopp(GPS_breddegrad, GPS_lengdegrad)
        buss_retning_tuple = retning(utgangspunkt)
        print(f'Destinasjon: {buss_retning_tuple[2]} og retning {buss_retning_tuple[1]}')
        print(type(buss_retning_tuple[1]))
        # global message_counter
        if message_counter > 0:
            search = True
            plan = False
                
# Denne funksjonen skal hente ut alle bussene på linje 3 og sjekke hvilken vei d gaar:
while search:
    # Sender GraphQL query
    vehicleResponse = sendGraphqlQuery(query)
    lengde = len(vehicleResponse['data']['vehicles'])
    # Itererer gjennom responsen fra API'et og sorterer ID'ene i to lister for å skille mellom retning
    for i in range(lengde):
        tempDir = vehicleResponse['data']['vehicles'][i]['direction']
        if tempDir == "Outbound":
            directionId = vehicleResponse['data']['vehicles'][i]
            Outbound.append(directionId['vehicleId'])
        else:
            directionId = vehicleResponse['data']['vehicles'][i]
            Inbound.append(directionId['vehicleId'])
    svar = buss_retning_tuple[1]
    if svar == "Inbound":
        dir = svar
        print(f'Du kan velge mellom disse bussene {Inbound}')
        sort_bus = True
        search = False
    elif svar == "Outbound":
        dir = svar
        print(f'Du kan velge mellom disse bussene {Outbound}')
        sort_bus = True
        search = False

    
# Sort_bus tilstand:
while sort_bus:
    # Sender nytt query
    vehicleResponse = sendGraphqlQuery(query)
    lengde = len(vehicleResponse['data']['vehicles'])
    # Itererer gjennom responsen og regner ut distansen fra start stoppet til klienten
    # Dersom bussen er innenfor rekkevidde og har riktig retning bryter programmet ut av denne løkken
    for i in range(lengde):
        temp = vehicleResponse['data']['vehicles'][i]
        distanse = distanse_kalk(breddegrad, lengdegrad, temp['location']['latitude'], temp['location']['longitude'])
        if distanse < rekkevidde and dir == "Inbound":
            global ID
            ID = temp['vehicleId']
            if ID in Inbound:
                ID = Inbound[Inbound.index(ID)]
                print(f'Buss: {ID}, distanse fra stoppet: {distanse}')
                pre_follow = True
                sort_bus = False
                break
        elif distanse < rekkevidde and dir == "Outbound":
            ID = temp['vehicleId']
            if ID in Outbound:
                ID = Outbound[Outbound.index(ID)]
                print(f'Buss: {ID}, distanse fra stoppet: {distanse}')
                pre_follow = True
                sort_bus = False
                break
        # Hvis ingen av betingenlesene er oppfylt vil programmet vente i 2 sekund og sjekke igjen
        elif distanse > rekkevidde:
            print('Venter paa buss...')
            time.sleep(2)
        
# Pre-follow tilstand:
while pre_follow:
    # Millis timer men med time biblioteket
    currmillis = time.time()
    if currmillis - prevmillis >= intervall:
        prevmillis = time.time()
        # Genererer nytt query basert på ID'en fra forige tilstand
        IdQuery = '''{
          vehicles(codespaceId: "ATB", lineRef: "ATB:Line:2_3", vehicleId: "''' + ID +  '''") {
            line {
              lineRef
              lineName
            }
            lastUpdated
            location {
              latitude
              longitude
            }
            direction
            vehicleId
          }
        }'''
        # Polling av nytt request hvert andre sekund:
        vehicle_lock_on = lock_on(IdQuery)
        lock_on_id = vehicle_lock_on['data']['vehicles'][0]['vehicleId']
        lock_on_latitude = vehicle_lock_on['data']['vehicles'][0]['location']['latitude']
        lock_on_longitude = vehicle_lock_on['data']['vehicles'][0]['location']['longitude']
        # regner ut distansen bussen har fra stoppet
        lock_on_distance = distanse_kalk(breddegrad, lengdegrad, lock_on_latitude, lock_on_longitude)
        print(f'Buss 3 med ID: {lock_on_id} er {lock_on_distance}m fra stoppet ditt')
        # Regner ut endringen i distanse
        if abs(lock_on_distance - prev_lock_on_distance) > 0:
            relative_direction = lock_on_distance-prev_lock_on_distance
            print(f'Distanse endring: {relative_direction}')
            # Kaller på mqtt client loopen for å kunne motta og sende meldinger
            client.loop(timeout=1.0, max_packets=None)
            prev_lock_on_distance = lock_on_distance
            # Publiserer distanse fra busstoppet til MQTT på topic "ESP"
            pos_message = str(lock_on_distance)
            publish_message("84.214.173.115", 1883, "ESP", pos_message)
            # Dersom retningen har endret seg og distansen fra stoppet er under 40 meter antar vi att bussen har passert
            if relative_direction > 0 and lock_on_distance < 40:
                print("Bussen har passert")
                follow = True
                pre_follow = False
                
# Løkke som følger bussen og sender distanse fra stoppet hvert 4. sekund på MQTT på topic "ESP"                
while follow:
    # Kaller på mqtt client loopen for å kunne motta og sende meldinger
    client.loop(timeout=1.0, max_packets=None)
    # Print for terminalen
    print('I følgemodus')
    time.sleep(intervall)
    vehicle_lock_on = lock_on(IdQuery)
    lock_on_latitude = vehicle_lock_on['data']['vehicles'][0]['location']['latitude']
    lock_on_longitude = vehicle_lock_on['data']['vehicles'][0]['location']['longitude']
    destinasjon_breddegrad = float(koordinat_data["Busstopp"][destinasjon_index]["Koordinater"][0])
    destinasjon_lengdegrad = float(koordinat_data["Busstopp"][destinasjon_index]["Koordinater"][1])
    lock_on_distance = distanse_kalk(lock_on_latitude, lock_on_longitude, destinasjon_breddegrad, destinasjon_lengdegrad)
    # Kontinuerlig oppdatering av klienten:
    pos_message = str(lock_on_distance)
    publish_message("84.214.173.115", 1883, "ESP", pos_message)
    # Prints for terminalen
    print(f'Sluttstopp breddegrad: {destinasjon_breddegrad}, lengdegrad: {destinasjon_lengdegrad}')
    print(f'Distanse til sluttstopp: {lock_on_distance}')
    # Dersom vi er innen 40m av stoppet anntar vi at klienten har nådd stoppet
    if lock_on_distance < 40:
        print("Vi er framme!")
        idle = True
        follow = False
