#include "volumnwidget.h"
#include "ui_volumnwidget.h"
#include <QDebug>

#include <stdio.h>
#include <alsa/asoundlib.h>

#include <unistd.h>
#include <fcntl.h>

#define VOLUME_BOUND 10000

typedef enum {
    AUDIO_VOLUME_SET,
    AUDIO_VOLUME_GET,
} audio_volume_action;

/*
  Drawbacks. Sets volume on both channels but gets volume on one. Can be easily adapted.
 */
int audio_volume(audio_volume_action action, long* outvol)
{
    int ret = 0;
    snd_mixer_t* handle;
    snd_mixer_elem_t* elem;
    snd_mixer_selem_id_t* sid;

    static const char* mix_name = "Volume";
    static const char* card = "default";
    static int mix_index = 0;

    long pmin, pmax;
    long get_vol, set_vol;
    float f_multi;

    snd_mixer_selem_id_alloca(&sid);

    //sets simple-mixer index and name
    snd_mixer_selem_id_set_index(sid, mix_index);
    snd_mixer_selem_id_set_name(sid, mix_name);

    if ((snd_mixer_open(&handle, 0)) < 0)
        return -1;
    if ((snd_mixer_attach(handle, card)) < 0) {
        snd_mixer_close(handle);
        return -2;
    }
    if ((snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
        snd_mixer_close(handle);
        return -3;
    }
    ret = snd_mixer_load(handle);
    if (ret < 0) {
        snd_mixer_close(handle);
        return -4;
    }

    for(elem=snd_mixer_first_elem(handle); elem; elem=snd_mixer_elem_next(elem))
    {
         if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE &&
             snd_mixer_selem_is_active(elem)) // ÕÒµœ¿ÉÒÔÓÃµÄ, Œ€»îµÄelem
        {
            //printf("------%s----------\n",snd_mixer_selem_get_name(elem));
            if(strcmp(snd_mixer_selem_get_name(elem),"DAC")==0)
            {
                break;
            }
        }
    }

    long minv, maxv;

    snd_mixer_selem_get_playback_volume_range (elem, &minv, &maxv);
    fprintf(stderr, "Volume range <%i,%i>\n", minv, maxv);

    if(action == AUDIO_VOLUME_GET) {
        if(snd_mixer_selem_get_playback_volume(elem, 0, outvol) < 0) {
            snd_mixer_close(handle);
            return -6;
        }

        fprintf(stderr, "Get volume %i with status %i\n", *outvol, ret);
        /* make the value bound to 100 */
        *outvol -= minv;
        maxv -= minv;
        minv = 0;
        *outvol = 100 * (*outvol) / maxv; // make the value bound from 0 to 100
    }
    else if(action == AUDIO_VOLUME_SET) {
        if(*outvol < 0 || *outvol > VOLUME_BOUND) // out of bounds
            return -7;
        *outvol = (*outvol * (maxv - minv) / (100)) + minv;

        if(snd_mixer_selem_set_playback_volume(elem, 0, *outvol) < 0) {
            snd_mixer_close(handle);
            return -8;
        }
        if(snd_mixer_selem_set_playback_volume(elem, 1, *outvol) < 0) {
            snd_mixer_close(handle);
            return -9;
        }
        fprintf(stderr, "Set volume %i with status %i\n", *outvol, ret);
    }

    snd_mixer_close(handle);
    return 0;
}


VolumnWidget::VolumnWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VolumnWidget)
{
    ui->setupUi(this);

    QDir  settingsDir("/data/");

    if(settingsDir.exists()){
        volumnFile = new QFile("/data/volumn");
    }else{
        volumnFile = new QFile("/etc/volumn");
    }

    volumnFile->open(QFile::ReadOnly | QIODevice::Truncate);
    QByteArray readAll= volumnFile->readAll();
    QString volumnString(readAll);
    long volumnInt= volumnString.toInt();
    qDebug()<<"read volumnInt from sys:"<<volumnInt;
    audio_volume(AUDIO_VOLUME_SET, &volumnInt);
    ui->m_VolumnHorizontalSlider->setValue(volumnInt);

}

VolumnWidget::~VolumnWidget()
{
    delete ui;
}


void VolumnWidget::onVolumnChange(int volumn){
    qDebug()<< "volumn:" << volumn;
    long int vol=volumn;
    audio_volume(AUDIO_VOLUME_SET, &vol);
    saveVolumn(volumn);
}

void VolumnWidget::on_m_VolumnDownPushButton_clicked()
{
   ui->m_VolumnHorizontalSlider->setValue(ui->m_VolumnHorizontalSlider->value()-ui->m_VolumnHorizontalSlider->pageStep());
}

void VolumnWidget::on_m_VolumnUpPushButton_clicked()
{
    ui->m_VolumnHorizontalSlider->setValue(ui->m_VolumnHorizontalSlider->value()+ui->m_VolumnHorizontalSlider->pageStep());
}

void VolumnWidget::saveVolumn(int volumn){

    QDir  settingsDir("/data/");

    if(settingsDir.exists()){
        volumnFile = new QFile("/data/volumn");
    }else{
        volumnFile = new QFile("/etc/volumn");
    }

    if (volumnFile->open(QFile::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(volumnFile);
        out <<volumn;
     }
}

void VolumnWidget::on_m_VolumnHorizontalSlider_sliderMoved(int position)
{
    onVolumnChange(position);
}


void VolumnWidget::on_m_VolumnHorizontalSlider_valueChanged(int value)
{
    onVolumnChange(value);
}